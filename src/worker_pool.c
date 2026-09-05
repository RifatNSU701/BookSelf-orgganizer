#include "worker_pool.h"

#include <pthread.h>
#include <stdlib.h>

#define BS_POOL_STOP 0
#define BS_POOL_RUN 1

typedef struct {
    BsWorkerTask task;
    void *context;
} BsWorkerJob;

struct BsWorkerPool {
    pthread_t *workers;
    BsWorkerJob *queue;
    size_t worker_count;
    size_t queue_capacity;
    size_t head;
    size_t tail;
    size_t queued;
    size_t active;
    int state;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_cond_t idle;
};

static void *worker_main(void *context)
{
    BsWorkerPool *pool = context;
    for (;;) {
        BsWorkerJob job;
        pthread_mutex_lock(&pool->mutex);
        while (pool->queued == 0 && pool->state == BS_POOL_RUN)
            pthread_cond_wait(&pool->not_empty, &pool->mutex);
        if (pool->queued == 0 && pool->state == BS_POOL_STOP) {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
        job = pool->queue[pool->head];
        pool->head = (pool->head + 1) % pool->queue_capacity;
        --pool->queued;
        ++pool->active;
        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->mutex);

        job.task(job.context);

        pthread_mutex_lock(&pool->mutex);
        --pool->active;
        if (pool->queued == 0 && pool->active == 0)
            pthread_cond_broadcast(&pool->idle);
        pthread_mutex_unlock(&pool->mutex);
    }
}

int bs_worker_pool_create(BsWorkerPool **pool, size_t worker_count, size_t queue_capacity)
{
    if (pool == NULL || worker_count == 0 || queue_capacity == 0) return -1;
    BsWorkerPool *instance = calloc(1, sizeof(*instance));
    if (instance == NULL) return -1;
    instance->workers = calloc(worker_count, sizeof(*instance->workers));
    instance->queue = calloc(queue_capacity, sizeof(*instance->queue));
    if (instance->workers == NULL || instance->queue == NULL) {
        free(instance->workers); free(instance->queue); free(instance); return -1;
    }
    instance->worker_count = worker_count;
    instance->queue_capacity = queue_capacity;
    instance->state = BS_POOL_RUN;
    if (pthread_mutex_init(&instance->mutex, NULL) != 0 ||
        pthread_cond_init(&instance->not_empty, NULL) != 0 ||
        pthread_cond_init(&instance->not_full, NULL) != 0 ||
        pthread_cond_init(&instance->idle, NULL) != 0) {
        free(instance->workers); free(instance->queue); free(instance); return -1;
    }
    size_t started = 0;
    for (; started < worker_count; ++started) {
        if (pthread_create(&instance->workers[started], NULL, worker_main, instance) != 0) {
            pthread_mutex_lock(&instance->mutex);
            instance->state = BS_POOL_STOP;
            pthread_cond_broadcast(&instance->not_empty);
            pthread_mutex_unlock(&instance->mutex);
            for (size_t i = 0; i < started; ++i) pthread_join(instance->workers[i], NULL);
            pthread_cond_destroy(&instance->idle); pthread_cond_destroy(&instance->not_full);
            pthread_cond_destroy(&instance->not_empty); pthread_mutex_destroy(&instance->mutex);
            free(instance->workers); free(instance->queue); free(instance); return -1;
        }
    }
    *pool = instance;
    return 0;
}

int bs_worker_pool_submit(BsWorkerPool *pool, BsWorkerTask task, void *context)
{
    if (pool == NULL || task == NULL) return -1;
    pthread_mutex_lock(&pool->mutex);
    while (pool->queued == pool->queue_capacity && pool->state == BS_POOL_RUN)
        pthread_cond_wait(&pool->not_full, &pool->mutex);
    if (pool->state != BS_POOL_RUN) { pthread_mutex_unlock(&pool->mutex); return -1; }
    pool->queue[pool->tail].task = task;
    pool->queue[pool->tail].context = context;
    pool->tail = (pool->tail + 1) % pool->queue_capacity;
    ++pool->queued;
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

int bs_worker_pool_wait(BsWorkerPool *pool)
{
    if (pool == NULL) return -1;
    pthread_mutex_lock(&pool->mutex);
    while (pool->queued != 0 || pool->active != 0)
        pthread_cond_wait(&pool->idle, &pool->mutex);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

void bs_worker_pool_shutdown(BsWorkerPool *pool)
{
    if (pool == NULL) return;
    pthread_mutex_lock(&pool->mutex);
    if (pool->state == BS_POOL_RUN) pool->state = BS_POOL_STOP;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    pthread_mutex_unlock(&pool->mutex);
}

void bs_worker_pool_destroy(BsWorkerPool *pool)
{
    if (pool == NULL) return;
    bs_worker_pool_shutdown(pool);
    for (size_t i = 0; i < pool->worker_count; ++i) pthread_join(pool->workers[i], NULL);
    pthread_cond_destroy(&pool->idle); pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->not_empty); pthread_mutex_destroy(&pool->mutex);
    free(pool->workers); free(pool->queue); free(pool);
}
