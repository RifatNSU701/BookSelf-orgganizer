#include "synchronization.h"
#include "logger.h"

#include <stdio.h>

int sync_init(SyncPrimitives *s)
{
    if (s == NULL) {
        return -1;
    }
    s->initialized = 0;

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
