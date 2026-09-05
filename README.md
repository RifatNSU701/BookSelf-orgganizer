# Bookshelf Management System

**Multithreaded and Multiprocess Bookshelf Management System Using Processes, POSIX Threads, Semaphores, and Mutex Locks**

An Operating Systems course project written in C. It organises a personal book
collection on a virtual bookshelf while demonstrating, with real OS primitives:

* **Processes** — genuine `fork()` / `waitpid()` (POSIX builds).
* **POSIX threads** — `pthread_create` / `pthread_join` for concurrent sorting,
  inventory update and search.
* **Mutex** — `pthread_mutex_t` protects the shared digital inventory.
* **POSIX semaphores** — `sem_init` / `sem_wait` / `sem_post` / `sem_destroy`
  order the phases of the organization pipeline.

---

## Features

* Load 20 sample books from `data/books.txt`.
* Display all books; add new books (validated input).
* Search by title, author, or genre (case-insensitive substring) and get the
  shelf location.
* Sort alphabetically, by genre, or by publication year — single-threaded
  (for comparison) **or** concurrently through the multithreaded pipeline.
* Assign books to physical shelves & positions, then display the bookshelf.
* **Process demonstration**: parent forks two children (organization &
  reporting), each with its own address space; parent `waitpid()`s and reads
  their exit codes.
* **Race-condition demonstration**: the same shared-counter workload run with
  and without a mutex, so the effect of mutual exclusion is visible.
* Thread-safe logging with real PIDs and thread ids to `logs/session.log`.
* Persist the inventory back to disk.

---

## Architecture (short version)

```
Menu (main.c)
  |
  |-- Concurrent organize (thread_manager.c)
  |      3 sorting threads --post--> [counting semaphore sort_done]
  |                                        |
  |                                  inventory thread waits x3
  |                                        |
  |                                  lock(mutex) -> sort + assign shelves -> unlock
  |                                        |
  |                                  post[inventory_done] --> search thread (read under lock)
  |
  |-- Process demo (process_manager.c)
  |      parent fork() -> child 1 (organize)  ]
  |                    -> child 2 (report)    ]  separate address spaces
  |      parent waitpid() both, reads exit codes
  |
  |-- Shared inventory (inventory.c) guarded by pthread_mutex_t
```

Full details are in `docs/architecture.md`, `docs/synchronization.md`,
`docs/algorithms.md`, `docs/testing.md` and `docs/project_report.md`.

---

## Requirements

* A C compiler (GCC / MinGW / Cygwin GCC).
* POSIX threads and POSIX semaphores.
* For the **genuine `fork()`** demonstration: a POSIX environment
  (Cygwin, MSYS2, Linux or macOS). See the Windows section below.

---

## Windows 11 Setup

This is the most important part of the setup for a Windows user.

### Why native MinGW cannot run `fork()`

`fork()` is a **POSIX system call**. It clones the calling process into a
parent and a child, each with its own copy of the address space. Windows does
not expose Unix `fork()` to ordinary programs, and **native MinGW does not
implement it** — there is no `<unistd.h>` `fork()` you can link against. Any
project that claims "MinGW fork()" is either using a POSIX layer underneath or
faking it. This project refuses to fake it: on a native-MinGW build, menu
option 9 prints an honest notice explaining the limitation instead of
pretending a process was created.

`pthreads` and POSIX semaphores, on the other hand, **are** available on MinGW
(via the winpthreads runtime bundled with modern MinGW-w64), so the
thread / mutex / semaphore parts run fully on native Windows.

### Which environment supports genuine `fork()`

| Environment                | pthreads | semaphores | genuine `fork()` |
|----------------------------|:--------:|:----------:|:----------------:|
| Native MinGW (MinGW-w64)   |   yes    |    yes     |    **no**        |
| **Cygwin GCC**             |   yes    |    yes     |    **yes**       |
| **MSYS2 (MSYS runtime)**   |   yes    |    yes     |    **yes**       |
| Linux / macOS              |   yes    |    yes     |    **yes**       |

**Recommendation for full marks:** build with **Cygwin GCC** (or MSYS2) so that
every required concept — including real processes — runs. Use native MinGW only
if you want to demonstrate the threads/semaphore/mutex portion.

### Option A — Code::Blocks with native MinGW (threads/mutex/semaphore)

1. Install **Code::Blocks with the MinGW bundle** (the "codeblocks-*-mingw"
   installer from codeblocks.org).
2. `File -> Open...` and choose `BookshelfManagement.cbp`.
3. Select the **Debug** or **Release** target (top toolbar).
4. **Build** (Ctrl+F9), then **Run** (Ctrl+F10).
5. Menu options 1–8, 10, 11 work fully. Option 9 prints the honest
   "no fork() on native MinGW" notice.

If the linker complains about pthreads, open
`Settings -> Compiler -> Linker settings` and confirm `-lpthread` is present
(the `.cbp` already adds it). Some MinGW builds prefer `-pthread`; if so, add
that instead under `Other linker options`.

### Option B — Code::Blocks with Cygwin GCC (adds genuine fork())

1. Install **Cygwin** (cygwin.com). In the Cygwin installer, select the
   packages: `gcc-core`, `make`, and `libpthread` (usually pulled in
   automatically).
2. Open Code::Blocks. Go to
   `Settings -> Compiler -> Selected compiler -> "Cygwin GCC"`.
   * If "Cygwin GCC" is not listed, choose "GNU GCC Compiler", click
     **Copy**, name it *Cygwin GCC*, then under **Toolchain executables** set
     the **Compiler's installation directory** to your Cygwin `\bin` folder
     (e.g. `C:\cygwin64\bin`) and set the C compiler to `gcc.exe`.
3. Open `BookshelfManagement.cbp`, pick the **Debug-Cygwin** or
   **Release-Cygwin** target (these are pre-configured to use the `cygwin`
   compiler), then **Build** and **Run**.
4. Menu option 9 now creates **real child processes** with distinct PIDs.

> **PATH note:** if you run the `.exe` outside the Cygwin shell, keep
> `cygwin1.dll` reachable (either run from the Cygwin terminal, or add
> `C:\cygwin64\bin` to your Windows `PATH`).

### Option C — MSYS2 / command line (simplest for a quick demo)

```bash
# In an MSYS2 MSYS shell (not the MinGW shell) for genuine fork():
pacman -S gcc make
cd /path/to/BookshelfManagement
make
make run
```

---

## Build & Run (POSIX / Makefile)

```bash
make          # builds bin/BookshelfManagement
make run      # builds and runs
make clean    # removes objects and the binary
```

Run **from the project root** so the relative paths `data/books.txt` and
`logs/session.log` resolve. In Code::Blocks the working directory is set to `.`
in the `.cbp`; if you moved things, set
`Project -> Properties -> Build targets -> Execution working dir` to the project
root.

---

## Menu

```
 1. Display all books
 2. Add a book
 3. Search for a book
 4. Sort alphabetically (single-thread)
 5. Sort by genre (single-thread)
 6. Sort by publication date (single-thread)
 7. Organize bookshelf (CONCURRENT: threads+sem+mutex)
 8. Display bookshelf
 9. Process demonstration (fork/wait)
10. Race-condition demo (with vs without mutex)
11. Save inventory to file
12. Exit
```

---

## Synchronization explanation

* **Mutex** (`Inventory.lock`) enforces **mutual exclusion** on the shared
  inventory: only one thread mutates or reads the book array at a time, so
  there are no lost updates or half-read records.
* **Counting semaphore** (`sort_done`) enforces **ordering**: the inventory
  thread waits until *all three* sorting threads have posted, then commits the
  final order. A mutex cannot express "wait for N completions"; a semaphore can.
* **Binary-style semaphore** (`inventory_done`) gates the search phase behind
  the inventory update, so search never runs on a stale/incomplete shelf layout.

---

## Data format

`data/books.txt`, one book per line:

```
Title|Author|Genre|Year
```

Lines starting with `#` and blank lines are ignored. Twenty sample books ship
with the project.

---

## Troubleshooting

* **`undefined reference to pthread_create` / `sem_init`** — the pthreads
  library is not being linked. Ensure `-lpthread` (or `-pthread`) is in the
  linker options. The `.cbp` and `Makefile` already include it.
* **Option 9 says fork() is not supported** — you built with native MinGW.
  Rebuild with a Cygwin/MSYS2 target (Option B/C above) for real processes.
* **`could not open data/books.txt`** — you ran the program from the wrong
  directory. Run from the project root, or fix the execution working dir.
* **`cygwin1.dll not found`** when double-clicking the exe — run from the
  Cygwin terminal or add `C:\cygwin64\bin` to `PATH`.
* **Garbled output / interleaved lines** — should not happen; logging is
  mutex-protected. If you added your own `printf`s inside threads, wrap them or
  route them through `log_event`.

---

## Known Windows limitations

* Genuine `fork()` requires Cygwin/MSYS2 (documented above). Native MinGW is
  fully supported for threads, mutex and semaphores only.
* Thread ids printed in logs are a truncated numeric handle derived from
  `pthread_self()`; they are stable within a run and adequate for demonstrating
  which thread did what.

---

## Academic honesty

Every required primitive is genuinely used — no decorative or fake code. The
`fork()` limitation on native MinGW is stated plainly rather than disguised.
The project was checked with `gcc -Wall -Wextra` (no warnings) and run through
its menu paths; it was **not** claimed to be compiled in any environment where
that was not actually done.
