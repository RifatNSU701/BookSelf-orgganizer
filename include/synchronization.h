#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <semaphore.h>

#define NUM_SORTERS 3

typedef struct {
    sem_t sort_done;
    sem_t inventory_done;
    int initialized;
} SyncPrimitives;

int sync_init(SyncPrimitives *s);
void sync_destroy(SyncPrimitives *s);

#endif
