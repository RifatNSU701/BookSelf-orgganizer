#include "thread_manager.h"
#include "sorting.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

typedef struct {
    Inventory      *inv;
    SyncPrimitives *sync;
    int             strategy;
    const char     *tag;
} SorterArg;

static void *sorter_thread(void *arg)
{
    SorterArg *sa = (SorterArg *)arg;
    Book      *copy;
    int        n;

    pthread_mutex_lock(&sa->inv->lock);
    n = sa->inv->count;
    copy = (Book *)malloc(sizeof(Book) * (n > 0 ? n : 1));
    if (copy != NULL && n > 0) {
        memcpy(copy, sa->inv->books, sizeof(Book) * n);
    }
    pthread_mutex_unlock(&sa->inv->lock);

    if (copy == NULL) {
        log_event(sa->tag, "ERROR: out of memory; sorter aborting.");
        sem_post(&sa->sync->sort_done);
        return NULL;
    }

    log_event(sa->tag, "Started sorting a private copy of %d book(s).", n);

    switch (sa->strategy) {
        case 0: sort_books_by_title(copy, n); break;
        case 1: sort_books_by_genre(copy, n); break;
        default: sort_books_by_year(copy, n); break;
    }

    log_event(sa->tag, "Sorting completed.");
    free(copy);

    log_event("SEMAPHORE", "%s posting sort-completion signal.", sa->tag);
    sem_post(&sa->sync->sort_done);
    return NULL;
}

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

    for (i = 0; i < NUM_SORTERS; i++) {
        sem_wait(&ia->sync->sort_done);
        log_event("Inventory", "Received sort-completion %d of %d.",
                  i + 1, NUM_SORTERS);
    }

    log_event("MUTEX", "Inventory thread acquiring inventory lock...");
    pthread_mutex_lock(&ia->inv->lock);
    log_event("MUTEX", "Inventory lock ACQUIRED. Updating shared inventory.");

    switch (ia->final_strategy) {
        case 0: inventory_sort_by_title_locked(ia->inv); break;
        case 1: inventory_sort_by_genre_locked(ia->inv); break;
        default: inventory_sort_by_year_locked(ia->inv); break;
    }
    inventory_assign_shelves_locked(ia->inv);

    pthread_mutex_unlock(&ia->inv->lock);
    log_event("MUTEX", "Inventory lock RELEASED.");

    log_event("SEMAPHORE", "Posting inventory-done signal for search phase.");
    sem_post(&ia->sync->inventory_done);
    return NULL;
}

static void *search_thread(void *arg)
{
    InventoryArg *ia = (InventoryArg *)arg;
    int placed = 0, i;

    log_event("Search", "Waiting for inventory-done via semaphore...");
    sem_wait(&ia->sync->inventory_done);
    log_event("Search", "Inventory-done received. Verifying placement.");

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

    sargs[0].inv = inv; sargs[0].sync = sync; sargs[0].strategy = 0; sargs[0].tag = "Alphabetical";
    sargs[1].inv = inv; sargs[1].sync = sync; sargs[1].strategy = 1; sargs[1].tag = "Genre";
    sargs[2].inv = inv; sargs[2].sync = sync; sargs[2].strategy = 2; sargs[2].tag = "PublicationDate";

    rc = pthread_create(&inv_tid, NULL, inventory_thread, &iarg);
    if (rc != 0) {
        log_event("Inventory", "ERROR: pthread_create failed (%d).", rc);
        return -1;
    }
    rc = pthread_create(&search_tid, NULL, search_thread, &iarg);
    if (rc != 0) {
        log_event("Search", "ERROR: pthread_create failed (%d).", rc);
        pthread_join(inv_tid, NULL);
        return -1;
    }

    for (i = 0; i < NUM_SORTERS; i++) {
        started[i] = 0;
        rc = pthread_create(&sorters[i], NULL, sorter_thread, &sargs[i]);
        if (rc != 0) {
            log_event(sargs[i].tag, "ERROR: pthread_create failed (%d).", rc);
            sem_post(&sync->sort_done);
        } else {
            started[i] = 1;
        }
    }

    for (i = 0; i < NUM_SORTERS; i++) {
        if (started[i]) {
            pthread_join(sorters[i], NULL);
        }
    }
    pthread_join(inv_tid, NULL);
    pthread_join(search_tid, NULL);

    ok = 1;
    printf("========================================\n");
    printf(" PIPELINE COMPLETE\n");
    printf("========================================\n");
    return ok ? 0 : -1;
}

#define RACE_THREADS 4
#define RACE_ITERATIONS 100000

typedef struct {
    long           *counter;
    pthread_mutex_t *lock;
} RaceArg;

static void *race_worker(void *arg)
{
    RaceArg *ra = (RaceArg *)arg;
    int i;
    for (i = 0; i < RACE_ITERATIONS; i++) {
        if (ra->lock != NULL) {
            pthread_mutex_lock(ra->lock);
            {
                long tmp = *ra->counter;
                tmp = tmp + 1;
                *ra->counter = tmp;
            }
            pthread_mutex_unlock(ra->lock);
        } else {
            volatile long tmp = *ra->counter;
            tmp = tmp + 1;
            sched_yield();
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
    return counter;
}
