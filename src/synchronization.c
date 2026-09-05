/*
 * synchronization.c
 * Initialise / destroy the POSIX semaphores that order the pipeline phases.
 *
 * sort_done      : counting semaphore, initial value 0. Each of the NUM_SORTERS
 *                  sorting threads posts once; the inventory thread waits
 *                  NUM_SORTERS times, so it only proceeds after ALL sorters are
 *                  done. A mutex could not express "wait for N completions".
 * inventory_done : value 0, used as a gate. The inventory thread posts it once
 *                  after committing the update; the search thread waits on it,
 *                  guaranteeing search runs strictly after the update.
 */

#include "synchronization.h"
#include "logger.h"

#include <stdio.h>

int sync_init(SyncPrimitives *s)
{
    if (s == NULL) {
        return -1;
    }
    s->initialized = 0;

    /* Second arg 0 => shared between threads of this process (not across
     * unrelated processes). Initial values are 0: nothing signalled yet. */
    if (sem_init(&s->sort_done, 0, 0) != 0) {
        return -1;
    }
    if (sem_init(&s->inventory_done, 0, 0) != 0) {
        sem_destroy(&s->sort_done);
        return -1;
    }
    s->initialized = 1;
    return 0;
}

void sync_destroy(SyncPrimitives *s)
{
    if (s == NULL || !s->initialized) {
        return;
    }
    sem_destroy(&s->sort_done);
    sem_destroy(&s->inventory_done);
    s->initialized = 0;
}
