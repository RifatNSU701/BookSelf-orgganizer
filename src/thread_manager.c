/*
 * thread_manager.c
 * The heart of the concurrency demonstration.
 *
 * PIPELINE
 *   3 sorting threads  --post(sort_done)-->  [counting semaphore]
 *          |                                        |
 *          | each sorts a PRIVATE copy              | inventory thread
 *          | (no shared-array race)                 | waits N times
 *          v                                        v
 *   inventory thread: lock(mutex) -> apply final sort + shelf assignment
 *                     -> unlock(mutex) -> post(inventory_done)
 *                                                    |
 *                                                    v
 *   search thread: wait(inventory_done) -> lock(mutex) -> read -> unlock
 *
 * WHY THIS DESIGN
 *   - Threads share the process address space, so all of them can see 'inv'.
 *   - The sorters must NOT all write the same array at once (that is a race),
 *     so each copies the data, sorts locally, and reports timing. Only ONE
 *     thread (the inventory thread) writes the shared inventory, and it does
 *     so inside the mutex-protected critical section.
 *   - The counting semaphore provides ORDERING ("wait until all 3 finished"),
 *     which a mutex cannot do. The mutex provides MUTUAL EXCLUSION on the
 *     shared inventory. Both are genuinely needed.
 */

#include "thread_manager.h"
#include "sorting.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>       /* sched_yield: widen the race window in the demo */

/* ---- Sorting worker ----------------------------------------------------- */

typedef struct {
    Inventory      *inv;
    SyncPrimitives *sync;
    int             strategy;    /* 0 title, 1 genre, 2 year */
    const char     *tag;
} SorterArg;

static void *sorter_thread(void *arg)
{
    SorterArg *sa = (SorterArg *)arg;
    Book      *copy;
    int        n;

    /* Read the current size under the lock, then take a private snapshot.
     * We copy inside the lock so the snapshot is internally consistent. */
    pthread_mutex_lock(&sa->inv->lock);
    n = sa->inv->count;
    copy = (Book *)malloc(sizeof(Book) * (n > 0 ? n : 1));
    if (copy != NULL && n > 0) {
        memcpy(copy, sa->inv->books, sizeof(Book) * n);
    }
    pthread_mutex_unlock(&sa->inv->lock);

    if (copy == NULL) {
        log_event(sa->tag, "ERROR: out of memory; sorter aborting.");
        /* Still post so the inventory thread is not blocked forever. */
        sem_post(&sa->sync->sort_done);
        return NULL;
    }

    log_event(sa->tag, "Started sorting a private copy of %d book(s).", n);

    /* Sort the PRIVATE copy: no race, because no other thread touches it. */
    switch (sa->strategy) {
        case 0: sort_books_by_title(copy, n); break;
        case 1: sort_books_by_genre(copy, n); break;
        default: sort_books_by_year(copy, n); break;
    }

    log_event(sa->tag, "Sorting completed.");
    free(copy);

    /* Signal completion via the counting semaphore. */
    log_event("SEMAPHORE", "%s posting sort-completion signal.", sa->tag);
    sem_post(&sa->sync->sort_done);
    return NULL;
}

/* ---- Inventory update worker ------------------------------------------- */

typedef struct {
    Inventory      *inv;
    SyncPrimitives *sync;
    int             final_strategy;
} InventoryArg;

static void *inventory_thread(void *arg)
{
    InventoryArg *ia = (InventoryArg *)arg;
    int           i;

    log_event("Inventory", "Waiting for all %d sorters via semaphore...",
              NUM_SORTERS);

    /* Wait for every sorter to finish. This is the ORDERING the semaphore
     * provides: the update cannot begin until sorting is complete. */
    for (i = 0; i < NUM_SORTERS; i++) {
        sem_wait(&ia->sync->sort_done);
        log_event("Inventory", "Received sort-completion %d of %d.",
                  i + 1, NUM_SORTERS);
    }

    log_event("MUTEX", "Inventory thread acquiring inventory lock...");
    pthread_mutex_lock(&ia->inv->lock);           /* enter critical section */
    log_event("MUTEX", "Inventory lock ACQUIRED. Updating shared inventory.");

    /* Apply the chosen final ordering to the REAL shared inventory, then
     * assign shelves. This is the single writer of shared state. */
    switch (ia->final_strategy) {
        case 0: inventory_sort_by_title_locked(ia->inv); break;
        case 1: inventory_sort_by_genre_locked(ia->inv); break;
        default: inventory_sort_by_year_locked(ia->inv); break;
    }
    inventory_assign_shelves_locked(ia->inv);

    pthread_mutex_unlock(&ia->inv->lock);         /* leave critical section */
    log_event("MUTEX", "Inventory lock RELEASED.");

    /* Gate the search phase behind the completed update. */
    log_event("SEMAPHORE", "Posting inventory-done signal for search phase.");
    sem_post(&ia->sync->inventory_done);
    return NULL;
}

/* ---- Search / verification worker -------------------------------------- */

static void *search_thread(void *arg)
{
    InventoryArg *ia = (InventoryArg *)arg;
    int placed = 0, i;

    log_event("Search", "Waiting for inventory-done via semaphore...");
    sem_wait(&ia->sync->inventory_done);
    log_event("Search", "Inventory-done received. Verifying placement.");

    /* Read under the lock: must not read a half-updated inventory. */
    pthread_mutex_lock(&ia->inv->lock);
    for (i = 0; i < ia->inv->count; i++) {
        if (ia->inv->books[i].shelf > 0) {
            placed++;
        }
    }
    pthread_mutex_unlock(&ia->inv->lock);

    log_event("Search", "Verification done: %d book(s) placed on shelves.",
              placed);
    return NULL;
}

/* ---- Public: run the full pipeline ------------------------------------- */

int run_organization_pipeline(Inventory *inv, SyncPrimitives *sync,
                              int final_strategy)
{
    pthread_t  sorters[NUM_SORTERS];
    pthread_t  inv_tid, search_tid;
    SorterArg  sargs[NUM_SORTERS];
    InventoryArg iarg;
    int        started[NUM_SORTERS];
    int        i, rc;
    int        ok = 0;

    if (inv == NULL || sync == NULL) {
        return -1;
    }

    printf("\n========================================\n");
    printf(" CONCURRENT ORGANIZATION PIPELINE\n");
    printf("========================================\n");

    iarg.inv = inv;
    iarg.sync = sync;
    iarg.final_strategy = final_strategy;

    /* Configure the three sorters. */
    sargs[0].inv = inv; sargs[0].sync = sync; sargs[0].strategy = 0; sargs[0].tag = "Alphabetical";
    sargs[1].inv = inv; sargs[1].sync = sync; sargs[1].strategy = 1; sargs[1].tag = "Genre";
    sargs[2].inv = inv; sargs[2].sync = sync; sargs[2].strategy = 2; sargs[2].tag = "PublicationDate";

    /* Start the consumer threads first so they are already waiting. */
    rc = pthread_create(&inv_tid, NULL, inventory_thread, &iarg);
    if (rc != 0) {
        log_event("Inventory", "ERROR: pthread_create failed (%d).", rc);
        return -1;
    }
    rc = pthread_create(&search_tid, NULL, search_thread, &iarg);
    if (rc != 0) {
        log_event("Search", "ERROR: pthread_create failed (%d).", rc);
        /* Unblock the inventory thread's dependents by joining what we can. */
        pthread_join(inv_tid, NULL);
        return -1;
    }

    /* Start the sorter (producer) threads. */
    for (i = 0; i < NUM_SORTERS; i++) {
        started[i] = 0;
        rc = pthread_create(&sorters[i], NULL, sorter_thread, &sargs[i]);
        if (rc != 0) {
            log_event(sargs[i].tag, "ERROR: pthread_create failed (%d).", rc);
            /* Post on behalf of the failed sorter so consumers don't hang. */
            sem_post(&sync->sort_done);
        } else {
            started[i] = 1;
        }
    }

    /* Join all sorters. */
    for (i = 0; i < NUM_SORTERS; i++) {
        if (started[i]) {
            pthread_join(sorters[i], NULL);
        }
    }
    /* Join consumers. */
    pthread_join(inv_tid, NULL);
    pthread_join(search_tid, NULL);

    ok = 1;
    printf("========================================\n");
    printf(" PIPELINE COMPLETE\n");
    printf("========================================\n");
    return ok ? 0 : -1;
}

/* ---- Controlled race-condition demonstration --------------------------- */

#define RACE_THREADS      4
#define RACE_ITERATIONS 100000

typedef struct {
    long           *counter;
    pthread_mutex_t *lock;   /* NULL => no protection (race) */
} RaceArg;

static void *race_worker(void *arg)
{
    RaceArg *ra = (RaceArg *)arg;
    int i;
    for (i = 0; i < RACE_ITERATIONS; i++) {
        if (ra->lock != NULL) {
            /* PROTECTED: the whole read-modify-write is one critical section,
             * so no two threads can interleave. Result is deterministic. */
            pthread_mutex_lock(ra->lock);
            {
                long tmp = *ra->counter;
                tmp = tmp + 1;
                *ra->counter = tmp;
            }
            pthread_mutex_unlock(ra->lock);
        } else {
            /* UNPROTECTED: we split the increment into an explicit
             * read -> modify -> write with a 'volatile' temporary, and then
             * voluntarily yield the CPU (sched_yield) BETWEEN the read and the
             * write. Yielding at exactly this point forces the scheduler to run
             * the other threads while this thread is holding a stale value in
             * 'tmp', so several threads read the same old counter and then all
             * write back old+1 -- the classic lost-update race. This makes the
             * race manifest on essentially every run, on any machine, instead
             * of only occasionally. The 'volatile' prevents the compiler from
             * collapsing the split back into a single atomic-looking add. */
            volatile long tmp = *ra->counter;
            tmp = tmp + 1;
            sched_yield();               /* widen the interleaving window */
            *ra->counter = tmp;
        }
    }
    return NULL;
}

long run_race_demo(int use_mutex)
{
    pthread_t       tids[RACE_THREADS];
    RaceArg         args[RACE_THREADS];
    pthread_mutex_t lock;
    long            counter = 0;
    int             i, started[RACE_THREADS];

    if (use_mutex) {
        if (pthread_mutex_init(&lock, NULL) != 0) {
            return -1;
        }
    }

    for (i = 0; i < RACE_THREADS; i++) {
        args[i].counter = &counter;
        args[i].lock    = use_mutex ? &lock : NULL;
        started[i] = (pthread_create(&tids[i], NULL, race_worker, &args[i]) == 0);
    }
    for (i = 0; i < RACE_THREADS; i++) {
        if (started[i]) {
            pthread_join(tids[i], NULL);
        }
    }

    if (use_mutex) {
        pthread_mutex_destroy(&lock);
    }
    return counter; /* expected: RACE_THREADS * RACE_ITERATIONS */
}
