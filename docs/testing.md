# Testing

This document describes how the Bookshelf Management System was tested and the
results observed. The full, numbered test matrix with exact inputs and expected
outputs is in `tests/test_cases.md`; this file explains the *approach* and
records the *verification results*.

---

## 1. How the project was verified

Because the target IDE is Code::Blocks on Windows but the primary development
happened on a POSIX toolchain, verification used the same GCC family the project
targets (`gcc`, C11, `-lpthread`). Three independent checks were run:

1. **Warning-clean compilation** with `-Wall -Wextra -std=c11`.
2. **Scripted functional runs** feeding menu sequences on `stdin` and inspecting
   `stdout` and `logs/session.log`.
3. **Dynamic analysis** with AddressSanitizer + UndefinedBehaviorSanitizer +
   LeakSanitizer (`-fsanitize=address,undefined`) across every menu path.

> Honesty note: these checks were performed on a Linux/GCC host, which provides
> the same POSIX `fork`, pthreads and semaphores the Cygwin/MSYS2 Windows build
> uses. The native-MinGW target compiles the identical code; only the `fork()`
> demo differs (it prints the honest "not supported" notice). No claim is made
> that the exact `.cbp` was opened inside Code::Blocks on Windows.

---

## 2. Compilation result

```
gcc -Wall -Wextra -std=c11 -Iinclude src/*.c -o bookshelf -pthread
```

Result: **exit code 0, zero warnings, zero errors.** Every translation unit
compiled; the link succeeded with the pthread library.

---

## 3. Memory / undefined-behaviour result

```
gcc -Wall -Wextra -std=c11 -Iinclude src/*.c -o bookshelf_asan -pthread \
    -fsanitize=address,undefined -g
ASAN_OPTIONS=detect_leaks=1 ./bookshelf_asan   # exercising add/sort/organize/
                                               # fork/race/search/save/display
```

Result: **no leaks, no buffer overflows, no undefined behaviour reported.**
All heap allocations (the per-sorter private copies) are freed; all
synchronization primitives are destroyed.

---

## 4. Functional verification (summary)

| Area                        | What was checked                                              | Result |
|-----------------------------|--------------------------------------------------------------|--------|
| Startup / load              | 20 books load from `data/books.txt`; count correct           | PASS   |
| Display all                 | Table renders all 20 records                                 | PASS   |
| Add a book                  | New id assigned; book appears; logged                        | PASS   |
| Search (title/author/genre) | Case-insensitive substring match; location shown             | PASS   |
| Missing-book search         | "No book matched" message, no crash                          | PASS   |
| Single-thread sorts (4/5/6) | Correct title / genre / year ordering                        | PASS   |
| Concurrent organize (7)     | 3 sorters → semaphore ×3 → mutex update → search gate        | PASS   |
| Bookshelf display (8)       | Books grouped by shelf & position                            | PASS   |
| Process demo (9)            | Parent forks 2 children, distinct PIDs, waitpid + exit codes | PASS   |
| Race demo (10)              | Without-mutex under-counts; with-mutex exact                 | PASS   |
| Save (11)                   | File rewritten in `Title|Author|Genre|Year`; reloads cleanly | PASS   |
| Invalid menu input          | Non-numeric / out-of-range re-prompts                        | PASS   |
| Empty string input          | Re-prompts, never stores empty title                         | PASS   |
| EOF (Ctrl-Z / closed stdin) | Clean shutdown, mutex destroyed, resources freed             | PASS   |

---

## 5. Concurrency-ordering evidence

The concurrent organize run (menu 7) reliably logs the expected causal order,
demonstrating the semaphore is doing real work:

```
[Inventory] Waiting for all 3 sorters via semaphore...
[Search]    Waiting for inventory-done via semaphore...
[Alphabetical] Sorting completed.      [SEMAPHORE] Alphabetical posting ...
[Inventory] Received sort-completion 1 of 3.
[Genre] Sorting completed.             [SEMAPHORE] Genre posting ...
[Inventory] Received sort-completion 2 of 3.
[PublicationDate] Sorting completed.   [SEMAPHORE] PublicationDate posting ...
[Inventory] Received sort-completion 3 of 3.
[MUTEX] Inventory lock ACQUIRED. Updating shared inventory.
[MUTEX] Inventory lock RELEASED.
[SEMAPHORE] Posting inventory-done signal for search phase.
[Search] Inventory-done received. Verifying placement.
[Search] Verification done: 20 book(s) placed on shelves.
```

The inventory update **never** begins before all three "sort-completion N of 3"
lines appear, and search **never** begins before "inventory-done received".
That ordering is enforced entirely by the two semaphores.

---

## 6. Race-condition evidence

Menu 10 runs four threads each incrementing a shared counter 100000 times
(expected total 400000):

```
WITHOUT mutex : ~200000-350000   (WRONG - lost updates due to race!)
WITH mutex    : 400000           (correct - mutual exclusion enforced)
```

The unprotected run splits the increment into an explicit read → modify → write
through a `volatile` temporary with a short spin loop between the read and the
write. This widens the window in which two threads read the same old value, so
lost updates appear on typical multi-core hardware (on a single-core or heavily
pinned machine the total may occasionally still be exact). Adding the mutex makes
the read-modify-write a single critical section and the total is always exact.

---

## 7. Process-isolation evidence

Menu 9 demonstrates that processes have separate address spaces:

```
[Parent] Parent process running (pid=1048). Forking children.
[Child-Organize] Organization child started (pid=1054, parent=1048).
[Child-Organize] Sorted my private copy by title. Parent's memory is untouched.
[Parent] Organization child (pid=1054) exited, code=0.
[Child-Report] Reporting child started (pid=1055, parent=1048).
[Child-Report] Report: 7 CS, 3 Fiction, 10 other (of 20).
[Parent] Reporting child (pid=1055) exited, code=0.
[Parent] All children reaped. Parent's inventory unchanged.
```

The distinct PIDs prove real processes were created; the "parent's memory is
untouched" line (verifiable by then choosing menu 1) proves address-space
isolation — the child's sort did not reorder the parent's in-memory inventory.

---

## 8. Regression checklist before delivery

* [x] Compiles with `-Wall -Wextra`, zero warnings.
* [x] No leaks / overflows under AddressSanitizer.
* [x] All 12 menu actions exercised.
* [x] Invalid input, empty input and EOF handled.
* [x] Semaphores initialised and destroyed each organize run.
* [x] Mutex initialised at startup, destroyed at shutdown.
* [x] Threads all joined; children all reaped.
* [x] Sample data file restored to pristine 20-book state for delivery.
