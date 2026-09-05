# Project Report

## Multithreaded and Multiprocess Bookshelf Management System Using Processes, POSIX Threads, Semaphores, and Mutex Locks

---

## 1. Title

**Multithreaded and Multiprocess Bookshelf Management System Using Processes,
POSIX Threads, Semaphores, and Mutex Locks** — an Operating Systems course
project implemented in C as a console application.

---

## 2. Introduction

An avid reader wants to keep a large personal book collection neatly organised
on a physical bookshelf, backed by a shared digital inventory that can be sorted,
searched and updated. Doing this well is, underneath, a classic operating-systems
problem: several tasks (different sorting strategies, an inventory updater, a
search/verification task) want to run **concurrently** and share the same data,
so the program must coordinate them without corrupting the inventory or losing
updates.

This project builds exactly that system and uses it as a vehicle to demonstrate
genuine OS concepts: **processes** (`fork`/`waitpid`), **POSIX threads**
(`pthread_create`/`pthread_join`), a **mutex** (`pthread_mutex_t`) protecting the
shared inventory, and **POSIX semaphores** (`sem_init`/`sem_wait`/`sem_post`)
ordering the phases of the organization pipeline.

---

## 3. Problem Statement

Organise a personal book collection on a virtual bookshelf while keeping a shared
digital inventory consistent under concurrent access. Specifically:

* sort books by multiple criteria (title, genre, publication year);
* keep a shared inventory of every book and its shelf location;
* protect that inventory from data inconsistencies when multiple concurrent
  tasks (or an automation system) touch it at once.

The solution must create multiple threads for the concurrent tasks, use a mutex
to protect the shared inventory, and use semaphores to synchronise the phases so
that, for example, the inventory update only runs after sorting has finished.

---

## 4. Objectives

* Demonstrate **process** creation, execution, synchronisation and termination
  with real `fork()`/`waitpid()`.
* Demonstrate **multithreading** with POSIX threads performing concurrent work.
* Protect a genuinely **shared resource** (the inventory) with a **mutex**.
* Order pipeline phases with **counting and binary semaphores**.
* Make the concurrency **visible** through logging with real PIDs/TIDs.
* Provide a controlled **race-condition demonstration** (with vs. without mutex).
* Keep the code modular, robust, leak-free and academically honest about the
  Windows `fork()` limitation.

---

## 5. System Requirements

* **OS:** Windows 11 (primary target); also runs on Linux/macOS.
* **IDE:** Code::Blocks (project file `BookshelfManagement.cbp` provided).
* **Compiler:** GCC (MinGW for threads/mutex/semaphore; Cygwin or MSYS2 for
  genuine `fork()`).
* **Libraries:** POSIX threads (`-lpthread`), POSIX semaphores, standard C
  library.
* **Language/standard:** C11.

---

## 6. Technologies Used

| Concept        | API / mechanism                                              |
|----------------|-------------------------------------------------------------|
| Processes      | `fork`, `waitpid`, `getpid`, `getppid`, `_exit`, exit status |
| Threads        | `pthread_create`, `pthread_join`, `pthread_self`            |
| Mutual exclusion | `pthread_mutex_t`, `pthread_mutex_lock/unlock/init/destroy` |
| Synchronisation | POSIX `sem_t`, `sem_init`, `sem_wait`, `sem_post`, `sem_destroy` |
| Race widening   | `volatile` temporary + short spin loop (in the demo only) |
| Storage        | Plain text file `data/books.txt`                            |
| Build          | Code::Blocks `.cbp` + `Makefile`                            |

---

## 7. Operating System Concepts Used

* **Process:** an executing program with its own address space. Created with
  `fork()`; the child is a near-copy of the parent that runs independently.
* **Thread:** a unit of execution *inside* a process that shares the process's
  address space. Created with `pthread_create()`.
* **pthreads:** the POSIX threads API — the standard, portable threading
  interface used throughout `thread_manager.c`.
* **Mutex:** a lock providing **mutual exclusion**; only one thread holds it at a
  time, so the code it guards (the **critical section**) runs atomically with
  respect to other threads. Protects the inventory in this project.
* **Semaphore:** a counter with atomic `wait` (decrement/block) and `post`
  (increment/wake) operations, used for **ordering/signalling** between tasks —
  something a mutex cannot express.
* **Synchronisation:** coordinating *when* tasks run relative to each other
  (semaphores) and *who* may touch shared data at a time (mutex).
* **Critical section:** the code that accesses shared state and must not be run
  by two threads at once — here, updating the inventory and assigning shelves.
* **Race condition:** a bug where the result depends on unlucky interleaving of
  unsynchronised accesses to shared data (demonstrated in menu option 10).
* **IPC (implicit):** the parent and children communicate through the shared
  **data file** and through **exit status** returned via `waitpid`.
* **Process synchronisation:** the parent blocks in `waitpid()` until each child
  terminates, then reads its exit code.

---

## 8. System Architecture

```
                    +---------------------------+
                    |        main.c (menu)      |
                    +---------------------------+
                       |        |         |
        single-thread  |        | concurrent organize (7)
        sorts (4/5/6)  |        |         |
                       v        v         v
             +---------+  +----------------+  +------------------+
             | sorting |  | thread_manager |  | process_manager  |
             | (pure)  |  | threads+sem    |  | fork()/waitpid() |
             +---------+  +----------------+  +------------------+
                              |     |
                    semaphores|     |mutex
                              v     v
                        +--------------------+
                        |     inventory      |  <-- THE shared resource
                        | Book[] + mutex     |
                        +--------------------+
                          ^     ^        ^
                          |     |        |
                    bookshelf  search   logger (PID/TID stamped)
                    (display)  (read)
```

Modules are split into `src/*.c` with matching `include/*.h`. Nothing lives in a
single monolithic file; `main.c` only wires modules together and drives the menu.

---

## 9. Process Architecture

```
Parent process (getpid = P)
   |
   |-- fork() --> Child 1: "Organization"  (pid = C1, parent = P)
   |                 loads inventory into ITS OWN memory, sorts by title,
   |                 _exit(0)
   |
   |-- fork() --> Child 2: "Reporting"      (pid = C2, parent = P)
   |                 loads inventory into ITS OWN memory, tallies genres,
   |                 _exit(0)
   |
   +-- waitpid(C1) -> read exit code
   +-- waitpid(C2) -> read exit code
   Parent's own in-memory inventory is UNCHANGED (separate address spaces).
```

**Why two children?** To show that each process has an independent address space:
the organization child sorts *its* copy without disturbing the parent, and the
reporting child computes a summary in *its* copy. Neither can corrupt the other
or the parent — the defining contrast with threads. `waitpid` + `WEXITSTATUS`
demonstrate process synchronisation and exit-status collection.

---

## 10. Thread Architecture

```
run_organization_pipeline():

  [Alphabetical]  [Genre]  [PublicationDate]   <- 3 producer threads
        \            |            /
         \  each sorts a PRIVATE copy (no shared-array race)
          \          |          /
           sem_post(sort_done) x3
                     |
                     v   (counting semaphore, waited 3x)
             [Inventory thread]                <- single writer
                lock(mutex)
                  apply final sort + assign shelves   (CRITICAL SECTION)
                unlock(mutex)
                sem_post(inventory_done)
                     |
                     v   (binary gate)
              [Search thread]                  <- reader
                sem_wait(inventory_done)
                lock(mutex) read/verify unlock
```

* **Sorting threads** read a consistent snapshot of the inventory (copied under
  the lock), sort their **private** copy, then signal completion. They never
  write shared state, so they cannot race with each other.
* **Inventory thread** is the **only writer**. It waits for all sorters, then
  updates the real inventory inside the mutex.
* **Search thread** waits for the update to finish, then reads under the lock.

All threads are created with `pthread_create` and reaped with `pthread_join`.

---

## 11. Synchronization Architecture

```
Sorting threads --post--> [ sort_done : counting semaphore, init 0 ]
                               |  Inventory thread waits 3 times
                               v
                       Inventory update
                          lock(mutex) ... unlock(mutex)
                               |
                               post
                               v
                    [ inventory_done : gate semaphore, init 0 ]
                               |  Search thread waits once
                               v
                          Search / verify (read under mutex)
```

* `sort_done` (counting semaphore) provides **ordering**: "proceed only after N
  completions." A mutex cannot express this.
* `inventory_done` (binary-style gate) provides a **happens-before** edge: search
  strictly follows the committed update.
* The **mutex** provides **mutual exclusion** on the shared inventory.

Full details and diagrams: `docs/synchronization.md`.

---

## 12. Data Structures

```c
typedef struct {
    int  id;
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char genre[MAX_GENRE_LEN];
    int  year;
    int  shelf;       /* 1-based, or -1 if unplaced */
    int  position;    /* 1-based, or -1 */
    BookStatus status; /* AVAILABLE / CHECKED_OUT */
} Book;

typedef struct {
    Book            books[MAX_BOOKS];
    int             count;
    pthread_mutex_t lock;   /* guards every field above */
} Inventory;

typedef struct {
    sem_t sort_done;        /* counting: one post per sorter */
    sem_t inventory_done;   /* gate for the search phase     */
    int   initialized;
} SyncPrimitives;
```

Bundling the mutex *inside* the `Inventory` struct is a deliberate design choice:
the lock lives with the data it protects, making the lock discipline obvious.

---

## 13. Sorting Algorithms

Three stable insertion sorts share one engine driven by a comparator: by title,
by genre (secondary key: title), and by year (secondary key: title). Insertion
sort is chosen for clarity and stability at this scale (~20 books). The routines
are pure (no locks, no I/O), which is what makes them safe to run on private
copies in worker threads and inside the inventory critical section. Full
analysis: `docs/algorithms.md`.

---

## 14. Inventory Management

The inventory is the shared digital catalogue. It supports: load from file, add
(thread-safe, mutex-protected), display, search by title/author/genre, sort
(three ways), assign shelf locations, display the bookshelf, and save back to
file. Every mutating operation runs with `inv->lock` held; read operations
(search, display) also take the lock so they never observe a half-updated
record.

---

## 15. Bookshelf Allocation

After a final ordering is chosen, books are laid out onto shelves of
`SHELF_CAPACITY` (8) positions across up to `MAX_SHELVES` (16) shelves:
`shelf = i / 8 + 1`, `position = i % 8 + 1`. Books beyond capacity are marked
*unplaced* rather than overflowing. Allocation mutates shared state and therefore
runs only inside the inventory thread's critical section.

---

## 16. Concurrency Design

Concurrency is introduced where it is *meaningful*, not decorative:

* **Multiple sorters** run at once, each on private data — genuine parallel work
  with no shared-write race.
* A **single writer** commits results, eliminating write-write races by design.
* **Readers** are synchronised with the writer through the mutex and gated behind
  the update through a semaphore.

This "many private producers → one synchronised writer → gated reader" shape is
the safest way to get concurrency without inviting corruption.

---

## 17. Mutex Design

* **What it protects:** the entire `Inventory` (book array, count, shelf fields).
* **Where it is taken:** inside `inventory_add`, the inventory thread's update,
  every search, and every display — i.e. every access to shared state.
* **Lifecycle:** `pthread_mutex_init` at startup (`inventory_init`),
  `pthread_mutex_destroy` at shutdown (`inventory_destroy`).
* **Critical section:** apply-final-sort + assign-shelves in the inventory
  thread. Two threads running this at once could interleave writes and produce an
  inconsistent shelf layout; the mutex makes it atomic.
* **Self-deadlock avoidance:** the sort/assign helpers are the `_locked`
  variants and assume the caller already holds the lock, so the code never tries
  to lock the same non-recursive mutex twice.

---

## 18. Semaphore Design

* `sort_done` — counting semaphore, initial value 0. Each of the 3 sorters posts
  once; the inventory thread waits 3 times, so it proceeds only after **all**
  sorters finish. This is ordering a mutex cannot provide.
* `inventory_done` — gate, initial value 0. Posted once after the update; the
  search thread waits once, guaranteeing search runs **after** the update.
* **Lifecycle:** `sem_init` before each organize run (so counts start at 0),
  `sem_destroy` after. Fresh primitives per run keep repeated demos correct.

---

## 19. Process Design

`fork()` is a POSIX system call. On POSIX builds (Cygwin/MSYS2/Linux/macOS) the
program creates two real child processes and reaps them with `waitpid`, reading
`WEXITSTATUS`. On **native MinGW**, which does **not** provide `fork()`, the code
is guarded by a compile-time `HAVE_FORK` macro and prints an **honest notice**
instead of faking a process. No thread is ever disguised as a process.

---

## 20. Error Handling

Every critical call is checked: `pthread_create`/`pthread_mutex_init`/`sem_init`
return values, `fork()` returning < 0, `malloc` returning NULL (the sorter still
posts its semaphore so no one hangs), `fopen` failures, and malformed data lines
(skipped defensively). If a sorter thread fails to start, the pipeline posts on
its behalf so consumers never block forever. Input is fully validated
(non-numeric, out-of-range, empty, over-long, EOF).

---

## 21. Testing

Verified by (1) warning-clean compilation under `-Wall -Wextra -std=c11`,
(2) scripted functional runs of every menu path, and (3) AddressSanitizer +
UBSan + LeakSanitizer showing no leaks/overflows/UB. Full results:
`docs/testing.md`; numbered matrix: `tests/test_cases.md`.

---

## 22. Sample Input

```
Loaded 20 book(s) from data/books.txt.
Enter choice: 7                 # concurrent organize
Choose final order: 1           # by title
Enter choice: 8                 # display bookshelf
Enter choice: 3                 # search
Enter field: 1                  # by title
Enter search text: Operating System Concepts
Enter choice: 9                 # process demo (POSIX build)
Enter choice: 10                # race demo
Enter choice: 12                # exit
```

---

## 23. Sample Output

```
========================================
 CONCURRENT ORGANIZATION PIPELINE
========================================
[PID=1048][TID=98432][Inventory] Waiting for all 3 sorters via semaphore...
[PID=1048][TID=13024][Alphabetical] Started sorting a private copy of 20 book(s).
[PID=1048][TID=13024][Alphabetical] Sorting completed.
[PID=1048][TID=98432][Inventory] Received sort-completion 1 of 3.
... (Genre, PublicationDate) ...
[PID=1048][TID=98432][MUTEX] Inventory lock ACQUIRED. Updating shared inventory.
[PID=1048][TID=98432][MUTEX] Inventory lock RELEASED.
[PID=1048][TID=5728][Search] Verification done: 20 book(s) placed on shelves.

Search: Operating System Concepts
  Title           : Operating System Concepts
  Author          : Abraham Silberschatz
  Genre           : Computer Science
  Publication Year: 2018
  Location        : Shelf 2, Position 3
  Status          : Available

WITHOUT mutex : 100001  (WRONG - lost updates due to race!)
WITH mutex    : 400000  (correct - mutual exclusion enforced)
```

---

## 24. Advantages

* Demonstrates all required OS concepts with genuine APIs.
* Race-free concurrency by design (private producers, single writer).
* Concurrency is **visible** via PID/TID-stamped logs.
* Modular, warning-clean, leak-free, well-commented C.
* Honest about the Windows `fork()` limitation.

---

## 25. Limitations

* Native MinGW cannot run the `fork()` demo (a POSIX limitation, not a bug);
  Cygwin/MSYS2 is required for that one feature.
* In-memory inventory capped at `MAX_BOOKS` (128); ample for a course project.
* File storage is plain text (by design — this is a synchronisation project, not
  a database project).
* Insertion sort is O(n^2) — irrelevant at this scale, but not suited to millions
  of books.

---

## 26. Future Improvements

* Reader–writer locks so multiple searches proceed in parallel while still
  excluding writers.
* A thread pool for sorting larger collections.
* Shared-memory IPC (`shm_open`/`mmap`) so children can return structured results
  to the parent instead of only exit codes.
* A checked-out / lending workflow using the existing `status` field.

---

## 27. Conclusion

The Bookshelf Management System organises a book collection while genuinely
exercising processes, POSIX threads, a mutex and semaphores. The mutex keeps the
shared inventory consistent; the semaphores order the pipeline phases; `fork()`
shows real process isolation; and the race-condition demo makes the *reason* for
mutual exclusion tangible. The result is a compact, honest, well-documented
project that a student can build in Code::Blocks and explain confidently in a
viva.

---

# Appendix A — Viva Questions & Answers (35)

**1. Why did you use processes?**
To demonstrate real OS process management and address-space isolation. The parent
`fork()`s two children that each load and manipulate the inventory in their own
memory; the parent's copy is untouched — something threads cannot show because
they share memory.

**2. Why did you use threads?**
The organization tasks (three sorts, an inventory update, a search) are
concurrent operations over the *same* in-memory data, so threads — which share
the address space — are the natural fit and avoid copying the whole inventory.

**3. What is the difference between a process and a thread?**
A process has its own address space and resources; a thread runs inside a process
and shares that address space with sibling threads. Processes are isolated
(safer, heavier); threads are lightweight and communicate through shared memory
(faster, but need synchronisation).

**4. Why use pthreads specifically?**
pthreads is the POSIX standard threading API — portable across Linux, macOS,
Cygwin and MSYS2, and the model the course teaches. It gives `pthread_create`,
`pthread_join`, mutexes and more in one standard interface.

**5. What is a mutex?**
A mutual-exclusion lock. Only one thread can hold it at a time, so the critical
section it guards runs atomically with respect to other threads.

**6. What is a semaphore?**
An integer counter with atomic `wait` (decrement, block if zero) and `post`
(increment, wake a waiter). It is used for signalling and ordering between tasks.

**7. Mutex vs semaphore — what's the difference?**
A mutex enforces mutual exclusion and has ownership (the locker unlocks). A
semaphore is a signalling counter with no ownership; a counting semaphore can
represent N permits. In this project the mutex protects the inventory; the
semaphores order the phases (mutex can't say "wait for 3 completions").

**8. What is a race condition?**
A bug where the outcome depends on the unpredictable interleaving of threads that
access shared data without synchronisation — e.g. two threads reading the same
counter and both writing back old+1, losing an update.

**9. What is a critical section?**
The region of code that accesses shared state and must not be executed by two
threads simultaneously.

**10. Where is the critical section in your project?**
In the inventory thread: applying the final sort and assigning shelves to the
real inventory, between `pthread_mutex_lock(&inv->lock)` and `unlock`. Search and
display also enter it to read consistently.

**11. What happens if you remove the mutex?**
Concurrent writers/readers could interleave mid-update, producing inconsistent
shelf assignments or a book with a half-written field. The race demo (option 10)
shows the concrete symptom: lost updates on a shared counter.

**12. Why is the semaphore necessary?**
To *order* phases: the inventory update must wait until all three sorts finish,
and search must wait until the update commits. A mutex only guarantees one-at-a-
time access; it cannot make one thread wait for another's completion.

**13. What happens if you remove the semaphore?**
The inventory thread could update before (or during) sorting, and search could
read before the update — breaking the intended happens-before ordering and
possibly reading stale or partial results.

**14. Why a counting semaphore for `sort_done`?**
Because three producers each signal completion once and one consumer must wait
for all three. Initialised to 0, posted 3 times, waited 3 times — a counting
semaphore expresses "wait for N events" directly.

**15. What is `fork()`?**
A POSIX system call that creates a new process by duplicating the caller. It
returns 0 in the child and the child's PID in the parent (or -1 on failure).

**16. What happens immediately after `fork()`?**
There are now two processes running the same code from the return point, each
with its own copy of memory. They diverge based on `fork()`'s return value.

**17. What is the parent process? The child process?**
The parent is the process that called `fork()`; the child is the newly created
duplicate. Here the parent coordinates and waits; the children do independent
work and exit.

**18. Why use `wait()`/`waitpid()`?**
So the parent blocks until a child finishes, collects its exit status, and
prevents zombie processes. We use `waitpid` to reap each specific child and read
`WEXITSTATUS`.

**19. How are threads synchronised in your project?**
By the mutex (mutual exclusion on the inventory) and two semaphores (`sort_done`
for "all sorts done", `inventory_done` for "update committed").

**20. Can two threads access the inventory simultaneously?**
Not for conflicting access: every access takes `inv->lock`, so writes are
exclusive and reads never see a half-written state. The lock serialises them.

**21. How are deadlocks prevented?**
There is essentially one lock (the inventory mutex) plus semaphores used in a
strict, one-directional order (sort → update → search). With a single lock there
is no lock-ordering cycle, and helpers assume-locked to avoid double-locking a
non-recursive mutex. So no thread waits on a cycle.

**22. What happens if two threads acquire locks in opposite orders?**
That is the classic deadlock scenario (A holds L1 waits L2; B holds L2 waits L1).
We avoid it by using a single inventory lock and a fixed semaphore order, so no
opposite-order acquisition exists.

**23. Why use both processes and threads?**
They demonstrate different OS ideas. Threads show shared-memory concurrency and
the need for mutex/semaphores. Processes show isolation and process management
(`fork`/`wait`/exit status). Using both makes the contrast explicit.

**24. Why isn't a database used?**
This is a synchronisation project, not a data project. A plain text file keeps
the focus on processes/threads/mutex/semaphores and keeps the build simple.

**25. How is the bookshelf location stored?**
Each `Book` has `shelf` and `position` integer fields (1-based, or -1 if
unplaced), set by `inventory_assign_shelves_locked` from the book's index in the
sorted list.

**26. How does the search operation work?**
It locks the inventory, linearly scans for a case-insensitive substring match on
the chosen field (title/author/genre), prints each match with its shelf location
and status, then unlocks.

**27. How does concurrent sorting work without a race?**
Each sorter copies the inventory under the lock into a *private* array, sorts
that copy (touching no shared memory), and signals completion. Only the single
inventory thread writes the shared inventory, so there is no write-write race.

**28. What is `pthread_self()` used for here?**
To obtain a thread identifier that the logger prints, so each log line shows
which thread produced it — making the concurrency observable.

**29. What does `getpid()`/`getppid()` show in the process demo?**
`getpid()` prints each process's own PID and `getppid()` the parent's, proving
the children are distinct processes with the parent as their parent.

**30. Why `_exit()` instead of `exit()` in the child?**
`_exit()` terminates immediately without flushing the C library buffers that were
duplicated from the parent, avoiding double-flushed output. It is the correct way
for a forked child to leave.

**31. What is mutual exclusion?**
The guarantee that at most one thread is inside the critical section at any time —
exactly what the mutex provides for the inventory.

**32. Is your mutex binary or counting? Your semaphores?**
A mutex is inherently binary (locked/unlocked) with ownership. `sort_done` is a
counting semaphore (value grows to 3); `inventory_done` is used as a binary gate
(0/1).

**33. What would happen on native MinGW at menu option 9?**
Because native MinGW lacks `fork()`, the `HAVE_FORK` guard compiles a fallback
that prints an honest "fork not supported" notice; the thread/mutex/semaphore
features still work. Genuine `fork()` requires Cygwin or MSYS2.

**34. How does the race demo make the race reliably visible?**
The unprotected worker splits the increment into an explicit read →
modify → write through a `volatile` temporary, with a small spin loop between
the read and the write. The `volatile` stops the compiler from collapsing the
three steps back into a single fetch-add, and the spin widens the window in
which another thread can read the same old value, so lost updates show up on
typical multi-core hardware. The protected worker wraps the whole
read-modify-write in `pthread_mutex_lock`/`unlock`, making it one critical
section, so its total is always correct.

**35. How do you ensure no memory leaks?**
Each sorter frees its private copy; semaphores are destroyed after each organize
run; the inventory mutex is destroyed at shutdown; all threads are joined and all
children reaped. This was verified under AddressSanitizer/LeakSanitizer.

---

# Appendix B — Demonstration Script (for the lab/viva)

Perform these steps in order; each maps to a concept the examiner will ask about.

1. **Build.** In Code::Blocks open `BookshelfManagement.cbp`, pick the target
   (Debug for MinGW threads/mutex/semaphore; Debug-Cygwin for genuine `fork()`),
   and Build & Run. Or from a shell: `make run`.

2. **Show the data loads.** Point out `Loaded 20 book(s) from data/books.txt.`
   Say: "This is the shared digital inventory."

3. **Display all books (menu 1).** Show the 20 records; note the Shelf/Pos column
   is `-` because nothing is organised yet.

4. **Concurrent organize (menu 7 → choose 1 = Title).** This is the centrepiece.
   Narrate the log as it scrolls:
   * three sorter threads start (distinct TIDs) — *"these run concurrently, each
     on a private copy, so they don't race"*;
   * `[SEMAPHORE] ... posting sort-completion` ×3 and
     `[Inventory] Received sort-completion N of 3` — *"the counting semaphore
     makes the update wait for all three"*;
   * `[MUTEX] Inventory lock ACQUIRED ... RELEASED` — *"this is the critical
     section protecting the shared inventory"*;
   * `[SEMAPHORE] Posting inventory-done` then `[Search] ... received` — *"the
     second semaphore gates the search phase behind the update."*

5. **Display bookshelf (menu 8).** Show books grouped into shelves/positions.

6. **Search (menu 3 → 1 → `Operating System Concepts`).** Show the full record
   with its shelf location — *"search reads under the same mutex."*

7. **Process demo (menu 9)** *(Cygwin/MSYS2 build).* Point at the two distinct
   child PIDs and the parent's `waitpid` lines with exit codes. Then choose menu
   1 again and note the parent's order is unchanged — *"separate address spaces."*
   (On a native-MinGW build, show the honest fork-not-supported notice instead
   and explain why.)

8. **Race demo (menu 10).** Read out `WITHOUT mutex` (wrong total, lost updates)
   vs `WITH mutex` (exactly 400000). Say: *"this is why the inventory needs the
   mutex."*

9. **Add a book (menu 2)** then organize again (menu 7) to show the new book
   placed on a shelf — demonstrates the full add→sort→place→search flow.

10. **Save (menu 11), then Exit (menu 12).** Show the clean shutdown lines
    ("destroying inventory mutex", "Goodbye") and mention `logs/session.log`
    holds the full PID/TID-stamped trace for the report.

**Examiner Q&A cues:** "Show me processes" → `src/process_manager.c` (`fork`,
`waitpid`). "Show me threads" → `src/thread_manager.c` (`pthread_create/join`).
"Show me the critical section" → the mutex-guarded update in
`inventory_thread`. "Why a semaphore?" → the `sort_done` count in the same file.
"Remove the mutex?" → menu 10's WITHOUT-mutex result.
