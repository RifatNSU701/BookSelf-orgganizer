#include "thread_manager.h"
#include "bookshelf_core.h"
#include "worker_pool.h"
#include "logger.h"

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define RACE_THREADS 4
#define RACE_ITERATIONS 100000
#define ORGANIZATION_WORKERS 3
#define ORGANIZATION_QUEUE 8

typedef struct {
    BsInventory *inventory;
    int strategy;
    const char *tag;
} SortTask;

static void sort_task(void *context)
{
    SortTask *task = context;
    BsInventory *snapshot = NULL;
    BsError result = bs_inventory_clone(task->inventory, &snapshot);
    if (result != BS_OK) {
        log_event(task->tag, "Snapshot failed: %s", bs_error_string(result));
        free(task);
        return;
    }

    if (task->strategy == 0) result = bs_inventory_sort_title(snapshot);
    else if (task->strategy == 1) result = bs_inventory_sort_genre(snapshot);
    else result = bs_inventory_sort_year(snapshot);

    log_event(task->tag, "Worker completed sort operation: %s", bs_error_string(result));
    bs_inventory_destroy(snapshot);
    free(task);
}

int run_organization_pipeline(BsInventory *inventory, int final_strategy)
{
    if (inventory == NULL || final_strategy < 0 || final_strategy > 2) return -1;

    BsWorkerPool *pool = NULL;
    if (bs_worker_pool_create(&pool, ORGANIZATION_WORKERS, ORGANIZATION_QUEUE) != 0) return -1;

    const char *tags[] = {"Alphabetical", "Genre", "PublicationDate"};
    for (int strategy = 0; strategy < 3; ++strategy) {
        SortTask *task = malloc(sizeof(*task));
        if (task == NULL) {
            bs_worker_pool_shutdown(pool);
            bs_worker_pool_destroy(pool);
            return -1;
        }
        task->inventory = inventory;
        task->strategy = strategy;
        task->tag = tags[strategy];
        if (bs_worker_pool_submit(pool, sort_task, task) != 0) {
            free(task);
            bs_worker_pool_shutdown(pool);
            bs_worker_pool_destroy(pool);
            return -1;
        }
    }

    if (bs_worker_pool_wait(pool) != 0) {
        bs_worker_pool_destroy(pool);
        return -1;
    }

    BsError result;
    if (final_strategy == 0) result = bs_inventory_sort_title(inventory);
    else if (final_strategy == 1) result = bs_inventory_sort_genre(inventory);
    else result = bs_inventory_sort_year(inventory);
    if (result == BS_OK) result = bs_inventory_assign_shelves(inventory);

    bs_worker_pool_destroy(pool);
    return result == BS_OK ? 0 : -1;
}

typedef struct {
    long *counter;
    pthread_mutex_t *lock;
} RaceArg;

static void *race_worker(void *context)
{
    RaceArg *arg = context;
    for (int i = 0; i < RACE_ITERATIONS; ++i) {
        if (arg->lock != NULL) {
            pthread_mutex_lock(arg->lock);
            ++(*arg->counter);
            pthread_mutex_unlock(arg->lock);
        } else {
            long value = *arg->counter;
            sched_yield();
            *arg->counter = value + 1;
        }
    }
    return NULL;
}

long run_race_demo(int use_mutex)
{
    pthread_t threads[RACE_THREADS];
    RaceArg args[RACE_THREADS];
    pthread_mutex_t lock;
    long counter = 0;
    int started = 0;

    if (use_mutex && pthread_mutex_init(&lock, NULL) != 0) return -1;
    for (int i = 0; i < RACE_THREADS; ++i) {
        args[i].counter = &counter;
        args[i].lock = use_mutex ? &lock : NULL;
        if (pthread_create(&threads[i], NULL, race_worker, &args[i]) != 0) break;
        ++started;
    }
    for (int i = 0; i < started; ++i) pthread_join(threads[i], NULL);
    if (use_mutex) pthread_mutex_destroy(&lock);
    return counter;
}
