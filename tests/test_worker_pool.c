#include "bookshelf_core.h"
#include "worker_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PRODUCERS 4
#define TASKS_PER_PRODUCER 500
#define WORKERS 4
#define QUEUE_CAPACITY 32

typedef struct { pthread_mutex_t mutex; long completed; } Counter;
typedef struct { Counter *counter; } TaskContext;

static void increment_task(void *context)
{
    TaskContext *task = context;
    pthread_mutex_lock(&task->counter->mutex);
    ++task->counter->completed;
    pthread_mutex_unlock(&task->counter->mutex);
    free(task);
}

static void *producer(void *context)
{
    BsWorkerPool *pool = ((BsWorkerPool **)context)[0];
    Counter *counter = ((Counter **)context)[1];
    for (int i = 0; i < TASKS_PER_PRODUCER; ++i) {
        TaskContext *task = malloc(sizeof(*task));
        if (task == NULL) return (void *)1;
        task->counter = counter;
        if (bs_worker_pool_submit(pool, increment_task, task) != 0) { free(task); return (void *)1; }
    }
    return NULL;
}

int main(void)
{
    BsWorkerPool *pool = NULL;
    Counter counter = {0};
    pthread_t producers[PRODUCERS];
    void *context[2] = {0};
    int failures = 0;

    pthread_mutex_init(&counter.mutex, NULL);
    if (bs_worker_pool_create(&pool, WORKERS, QUEUE_CAPACITY) != 0) {
        fprintf(stderr, "Unable to create worker pool.\n");
        pthread_mutex_destroy(&counter.mutex);
        return EXIT_FAILURE;
    }
    context[0] = pool;
    context[1] = &counter;
    for (int i = 0; i < PRODUCERS; ++i)
        if (pthread_create(&producers[i], NULL, producer, context) != 0) ++failures;
    for (int i = 0; i < PRODUCERS; ++i) pthread_join(producers[i], NULL);
    if (bs_worker_pool_wait(pool) != 0) ++failures;

    pthread_mutex_lock(&counter.mutex);
    long expected = (long)PRODUCERS * TASKS_PER_PRODUCER;
    long completed = counter.completed;
    pthread_mutex_unlock(&counter.mutex);
    if (completed != expected) {
        fprintf(stderr, "Expected %ld completed tasks, got %ld.\n", expected, completed);
        ++failures;
    }

    bs_worker_pool_destroy(pool);
    pthread_mutex_destroy(&counter.mutex);
    if (failures != 0) return EXIT_FAILURE;
    printf("Worker-pool stress test passed: %ld tasks completed.\n", completed);
    return EXIT_SUCCESS;
}
