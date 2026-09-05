#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

/*
 * synchronization.h
 * Central home for the POSIX semaphores used to order the phases of the
 * organization pipeline:
 *
 *      sorting phase  --(sem_post x3)-->  sem_sort_done  --(sem_wait x3)-->
 *      inventory update phase  --(sem_post)-->  sem_inventory_done
 *      --(sem_wait)-->  search / report phase
 *
 * A counting semaphore (sem_sort_done) lets the inventory thread wait until
 * ALL three sorting threads have finished, without busy-waiting. A second
 * semaphore (sem_inventory_done) gates the search phase behind the inventory
 * update. This is genuine producer/consumer style ordering that a plain mutex
 * cannot express (a mutex provides mutual exclusion, not ordering/signalling).
 */

#include <semaphore.h>

/* Number of sorting worker threads that must complete before inventory update. */
#define NUM_SORTERS 3

typedef struct {
    sem_t sort_done;        /* counting semaphore, posted once per sorter   */
    sem_t inventory_done;   /* binary-style gate for the search/report phase */
    int   initialized;      /* guards destroy against double-free           */
} SyncPrimitives;

/* Initialise both semaphores. Returns 0 on success, -1 on failure. */
int  sync_init(SyncPrimitives *s);

/* Destroy both semaphores (safe to call once after sync_init succeeds). */
void sync_destroy(SyncPrimitives *s);

#endif /* SYNCHRONIZATION_H */
