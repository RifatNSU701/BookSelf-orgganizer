#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

/*
 * thread_manager.h
 * Orchestrates the concurrent organization pipeline using POSIX threads,
 * a shared mutex (inside the Inventory) and POSIX semaphores.
 *
 * Threads created:
 *   - 3 sorting threads (title / genre / year). Each sorts a PRIVATE copy of
 *     the book array so they never race on the same memory, then posts the
 *     sort_done semaphore.
 *   - 1 inventory thread. It waits on sort_done NUM_SORTERS times, then locks
 *     the inventory mutex, applies the final ordering + shelf assignment
 *     (the critical section), unlocks, and posts inventory_done.
 *   - 1 search thread. It waits on inventory_done, then performs a read under
 *     the same mutex, proving readers are synchronised with writers.
 *
 * This function blocks until all threads have joined.
 */

#include "inventory.h"
#include "synchronization.h"

/*
 * run_organization_pipeline
 * Runs the full multithreaded organize sequence on 'inv'. 'final_strategy'
 * selects which sorted order becomes the shelf order:
 *   0 = title, 1 = genre, 2 = year.
 * Returns 0 on success, -1 if any thread failed to start.
 */
int run_organization_pipeline(Inventory *inv, SyncPrimitives *sync,
                              int final_strategy);

/*
 * run_race_demo
 * Controlled demonstration of a race condition on a shared counter.
 * If 'use_mutex' is non-zero the shared counter is protected and the result
 * is deterministic; otherwise the increments race and the total is usually
 * wrong (lost updates). This is a SAFE, self-contained demo (it does not
 * corrupt the real inventory). Returns the final counter value. On a
 * single-core / heavily-pinned machine the unprotected run may occasionally
 * still total correctly; on typical multi-core hardware it under-counts.
 */
long run_race_demo(int use_mutex);

#endif /* THREAD_MANAGER_H */
