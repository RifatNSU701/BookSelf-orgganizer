# System Architecture

## 1. Module map

```
include/ , src/
  main.c ................ menu, wiring, lifecycle
  inventory.[ch] ........ shared digital inventory + its mutex (the shared resource)
  sorting.[ch] .......... pure, reusable sort routines (no locks, no I/O)
  bookshelf.[ch] ........ physical-shelf rendering of the inventory
  search.[ch] ........... read operations over the inventory (under the lock)
  synchronization.[ch] .. POSIX semaphores used to order pipeline phases
  thread_manager.[ch] ... the concurrent organize pipeline + race demo
  process_manager.[ch] .. fork()/wait() process demo (honest MinGW fallback)
  input.[ch] ............ robust console input (invalid input, empty, EOF)
  logger.[ch] ........... thread-safe logging with real pid/tid
  utils.[ch] ............ safe strings, trimming, case-insensitive compare
```

Design principle: **the sort routines are pure**, so the same code is used by
single-threaded menu actions and by worker threads inside a critical section.
Only one module (`inventory.c`) owns the shared state and its lock, which keeps
the lock discipline in one place.

## 2. Overall system flowchart

```
              +---------------------------+
              |        Program start      |
              +-------------+-------------+
                            |
                 logger_init / inventory_init
                            |
                 inventory_load(data/books.txt)
                            |
                   +--------v---------+
                   |    Main menu     |<--------------------+
                   +--------+---------+                     |
                            |                               |
     +----------+----------+----------+----------+          |
     |          |          |          |          |          |
   Display    Add/Search  Single    Organize   Process/     |
   /Shelf     (mutex)     sort      (threads+  Race demo     |
                          (mutex)    sem+mutex)               |
     |          |          |          |          |            |
     +----------+----------+----------+----------+------------+
                            |
                       choice == Exit
                            |
              inventory_destroy / logger_close
                            |
                        Program end
```

## 3. Process architecture

Processes are used to demonstrate **separate address spaces** and OS process
management. Threads could not show this: threads share memory, processes do not.

```
                 Parent process (pid P)
                 loads/holds the in-memory inventory
                          |
             fork() ------+------ fork()
             |                          |
   Child 1 (pid C1)             Child 2 (pid C2)
   "Organization child"         "Reporting child"
   - loads books.txt into       - loads books.txt into
     ITS OWN memory               ITS OWN memory
   - sorts its private copy      - computes a genre tally
   - _exit(code)                 - _exit(code)
             |                          |
             +------------+-------------+
                          |
             Parent: waitpid(C1); waitpid(C2)
             reads WEXITSTATUS of each
             Parent's own inventory is UNCHANGED
```

Key teaching point: because each child sorts a **private** copy, the parent's
inventory is untouched after both children exit — proving processes have
independent memory. This is the honest reason to use processes here rather than
"because the assignment said process".

### Portability of the process demo

`fork()` is POSIX. It is detected at compile time:

```c
#if defined(__CYGWIN__) || defined(__unix__) || defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
#  define HAVE_FORK 1
#else
#  define HAVE_FORK 0     /* e.g. native MinGW */
#endif
```

On `HAVE_FORK == 0` (native MinGW) the demo prints an explicit notice and
returns without faking a process. On Cygwin/MSYS2/Linux/macOS it runs the real
`fork()`/`waitpid()` sequence above.

## 4. Thread architecture

Threads are used for the concurrent **organization pipeline**, where several
tasks run at once and share the inventory in memory.

```
                run_organization_pipeline()
                          |
   create inventory thread (consumer, waits on sort_done x3)
   create search thread    (consumer, waits on inventory_done)
                          |
   create 3 sorter threads (producers)
      +------------------+------------------+
      |                  |                  |
 Alphabetical         Genre           PublicationDate
 sorts a PRIVATE      sorts a         sorts a PRIVATE
 copy, then           PRIVATE copy    copy, then
 sem_post(sort_done)  sem_post        sem_post
      |                  |                  |
      +--------- counting semaphore --------+
                          | (value reaches 3)
                Inventory thread wakes
                lock(mutex)
                  apply final chosen order to REAL inventory
                  assign shelves/positions
                unlock(mutex)
                sem_post(inventory_done)
                          |
                Search thread wakes
                lock(mutex) -> read/verify -> unlock(mutex)
```

Why each thread exists:

| Thread            | Reads                      | Writes                         | Sync used                    |
|-------------------|----------------------------|--------------------------------|------------------------------|
| Alphabetical      | private copy of books      | private copy                   | posts `sort_done`            |
| Genre             | private copy of books      | private copy                   | posts `sort_done`            |
| PublicationDate   | private copy of books      | private copy                   | posts `sort_done`            |
| Inventory update  | shared inventory           | shared inventory (order+shelf) | waits `sort_done` x3; mutex; posts `inventory_done` |
| Search/verify     | shared inventory           | nothing                        | waits `inventory_done`; mutex |

The sorters intentionally do **not** write the shared array (that would be a
race). They demonstrate concurrent sorting; the single **inventory thread** is
the only writer of shared state, inside the mutex.

## 5. Data flow summary

```
data/books.txt --load--> Inventory (shared, mutex-guarded)
Inventory --snapshot--> sorter threads (private copies)
sorter completion --semaphore--> inventory thread
inventory thread --mutex--> Inventory (final order + shelves)
Inventory --semaphore--> search thread --mutex--> read
Inventory --save--> data/books.txt (menu option 11)
```
