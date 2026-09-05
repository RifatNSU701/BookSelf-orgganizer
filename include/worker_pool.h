#ifndef BOOKSHELF_WORKER_POOL_H
#define BOOKSHELF_WORKER_POOL_H

#include <stddef.h>

typedef void (*BsWorkerTask)(void *context);

typedef struct BsWorkerPool BsWorkerPool;

int bs_worker_pool_create(BsWorkerPool **pool, size_t worker_count, size_t queue_capacity);
int bs_worker_pool_submit(BsWorkerPool *pool, BsWorkerTask task, void *context);
int bs_worker_pool_wait(BsWorkerPool *pool);
void bs_worker_pool_shutdown(BsWorkerPool *pool);
void bs_worker_pool_destroy(BsWorkerPool *pool);

#endif
