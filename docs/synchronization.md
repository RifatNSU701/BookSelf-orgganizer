# Synchronization Design

## 1. The shared resource

The shared resource is the **digital inventory** (`Inventory` in
`inventory.h`): an array of `Book` plus a `count`. Multiple threads may touch it
at once (the inventory-update thread writes; the search thread reads; menu
actions read/write). Uncontrolled concurrent access would cause:

* **lost updates** (two writers interleave a read-modify-write),
* **half-read records** (a reader sees a book mid-update),
* **inconsistent shelf positions**.

## 2. Mutex — mutual exclusion on the inventory

The `Inventory` bundles a `pthread_mutex_t lock` with the data it protects.
Every mutating and reading path takes it:

```c
pthread_mutex_lock(&inv->lock);     /* enter critical section */
    ... modify or read inv->books / inv->count ...
pthread_mutex_unlock(&inv->lock);   /* leave critical section */
```

### The critical section

The most important critical section is in `thread_manager.c`,
`inventory_thread()`:

```c
pthread_mutex_lock(&ia->inv->lock);
    inventory_sort_by_*_locked(ia->inv);   /* reorder shared array   */
    inventory_assign_shelves_locked(ia->inv); /* set shelf/position  */
pthread_mutex_unlock(&ia->inv->lock);
```

Between lock and unlock, no other thread can read or write the inventory, so the
final order and shelf assignment are applied atomically from every other
thread's point of view.

### What happens without the mutex

Menu option 10 demonstrates this on a shared counter. Four threads each
increment a shared `long` 100000 times. The correct total is 400000.

* **Without** the mutex the increment is a non-atomic read-modify-write; two
  threads can read the same old value and both write back `old+1`, losing one
  update. The observed total is typically **below** 400000.
* **With** the mutex the increment is a critical section, so the total is
  **always** 400000.

This is exactly the failure mode that would corrupt the inventory if its lock
were removed.

## 3. Semaphores — ordering the pipeline

A mutex gives mutual exclusion but **not ordering**. The pipeline needs
ordering: the inventory update must not start until sorting has finished, and
search must not start until the update has finished. Two POSIX semaphores
provide this.

### `sort_done` — counting semaphore

* Initial value **0**.
* Each of the `NUM_SORTERS` (3) sorting threads calls `sem_post(&sort_done)`
  when it finishes.
* The inventory thread calls `sem_wait(&sort_done)` **NUM_SORTERS times**.

Because it waits three times, the inventory thread proceeds only after **all
three** sorters have completed — a "wait for N completions" barrier that a
single mutex cannot express. This is a producer/consumer relationship: sorters
produce completion tokens; the inventory thread consumes three of them.

### `inventory_done` — gate for the search phase

* Initial value **0**.
* The inventory thread calls `sem_post(&inventory_done)` once, after committing
  the update.
* The search thread calls `sem_wait(&inventory_done)` before reading.

This guarantees the search/verify phase runs strictly **after** the inventory is
consistent.

## 4. Synchronization diagram

```
 Sorting threads (x3)
      |  each: sem_post(sort_done)
      v
 [ counting semaphore sort_done ]   value climbs 0 -> 3
      |
      |  inventory thread: sem_wait(sort_done) x3   (blocks until all done)
      v
 Inventory update thread
      |  pthread_mutex_lock(inventory.lock)     <-- ENTER critical section
      v
 Shared Inventory  (reorder + assign shelves)
      |  pthread_mutex_unlock(inventory.lock)   <-- LEAVE critical section
      |  sem_post(inventory_done)
      v
 [ semaphore inventory_done ]
      |
      |  search thread: sem_wait(inventory_done)
      v
 Search / verify thread
      |  pthread_mutex_lock(inventory.lock) -> read -> unlock
      v
 Consistent result displayed
```

Every arrow:

1. **Sorters -> sort_done**: each finished sorter signals completion.
2. **sort_done -> inventory thread**: the inventory thread blocks until it has
   consumed three completion signals (all sorters done).
3. **inventory thread -> mutex**: it locks before touching shared state.
4. **mutex -> shared inventory**: reorder + shelf assignment happen atomically.
5. **shared inventory -> mutex unlock**: release so others may proceed.
6. **inventory thread -> inventory_done**: signal the update is committed.
7. **inventory_done -> search thread**: search unblocks only now.
8. **search thread -> mutex**: even the reader locks, so it never sees a
   partial update.

## 5. Why a semaphore instead of only a mutex?

* A mutex answers "**who** may touch the data now?" (one thread at a time).
* A semaphore answers "**when** may this phase begin?" (after N signals).

The pipeline needs both. Removing the semaphore would let the inventory thread
run before sorting finished (wrong/again ordering); removing the mutex would let
threads corrupt the shared array. They solve different problems.

## 6. Deadlock avoidance

Deadlock needs a circular wait on resources. This project avoids it by design:

* **One lock only** for shared state (`inventory.lock`). With a single mutex
  there is no lock-ordering problem and no circular wait among inventory locks.
* **Semaphores are always posted**, including on error paths: if a sorter
  thread fails to start, the pipeline still calls `sem_post(&sort_done)` on its
  behalf so the inventory thread cannot block forever. Likewise a memory-failed
  sorter posts before returning.
* **No thread holds the mutex while waiting on a semaphore.** The inventory
  thread does all its `sem_wait`s *before* taking the mutex, and posts
  `inventory_done` *after* releasing it. So a lock is never held across a
  potentially blocking wait.
* **The logger uses its own independent mutex** and never calls back into
  inventory code while holding it, so the two mutexes cannot form a cycle.

Consistent single-lock usage plus "signal-even-on-error" keeps the system
deadlock-free and livelock-free.
