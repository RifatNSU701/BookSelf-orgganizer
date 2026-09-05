<div align="center">

# BookShelf Organizer

### A production-minded, multithreaded bookshelf management system in C

**Native Windows binary • SQLite persistence • POSIX threading • Synchronization demonstrations**

[![Language](https://img.shields.io/badge/C-C11-00599C?style=for-the-badge&logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Build System](https://img.shields.io/badge/CMake-4.x-064F8C?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![Database](https://img.shields.io/badge/SQLite-3.x-003B57?style=for-the-badge&logo=sqlite)](https://sqlite.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2011-0078D4?style=for-the-badge&logo=windows)](https://www.microsoft.com/windows/)
[![Tests](https://img.shields.io/badge/Tests-2%2F2%20Passing-2EA44F?style=for-the-badge)](#testing)

A complete Operating Systems project demonstrating real processes, POSIX threads, mutexes, semaphores, concurrent work, persistent storage, logging, and bookshelf organization through a clean command-line application.

<br>

### Download the Windows Application

**[⬇ Download `BookShelfOrganizer.exe`](https://raw.githubusercontent.com/RifatNSU701/BookSelf-orgganizer/main/build/BookShelfOrganizer.exe)**

> **Important:** The executable is built against SQLite and expects the application's runtime/data environment to be available. For the most reliable experience, use the executable from a clone of this repository and run it from the repository root. A fully portable release package with bundled runtime DLLs/installer is the next packaging step.

</div>

---

## Overview

**BookShelf Organizer** is a C11-based bookshelf management application created to demonstrate practical Operating Systems concepts through a working system rather than isolated examples.

The application combines:

- Persistent SQLite-backed storage
- Book inventory management
- Search and sorting
- Shelf and position assignment
- Multithreaded organization
- Mutex-protected shared state
- POSIX semaphore synchronization
- Process creation and synchronization
- Race-condition demonstration
- Thread-safe logging
- Automated tests
- CMake-based builds

The project is structured so that the core functionality is separated from the command-line interface, storage layer, synchronization layer, and test targets.

---

## Key Features

### 📚 Inventory Management

- Load and manage a book collection
- Add new books with validated input
- Display the complete inventory
- Search by title, author, or genre
- Case-insensitive substring matching
- Track shelf and position information
- Persist inventory data

### ⚡ Concurrent Organization

The organization pipeline demonstrates real concurrency using multiple worker threads.

- Concurrent sorting operations
- Inventory updates protected by a mutex
- Semaphore-controlled phase ordering
- Search performed after the inventory phase completes
- Worker-pool based execution

### 🔐 Synchronization

The project demonstrates why different synchronization primitives exist and when each should be used.

| Primitive | Purpose |
|---|---|
| `pthread_mutex_t` | Protect shared inventory state |
| POSIX semaphore | Coordinate completion/order between phases |
| `pthread_create()` | Execute concurrent tasks |
| `pthread_join()` | Wait for worker completion |
| `fork()` / `waitpid()` | Demonstrate parent/child processes on POSIX environments |

### 🧪 Race-Condition Demonstration

The application includes a controlled shared-counter demonstration that compares execution with and without mutual exclusion, making the effects of synchronization visible.

### 📝 Logging

Thread-safe session logging records process and thread information to:

```text
logs/session.log
```

### 💾 Persistent Storage

SQLite is used as the persistent storage layer, allowing inventory data to survive application restarts.

---

## Architecture

```text
                           ┌─────────────────────┐
                           │      main.c         │
                           │    CLI / Menu       │
                           └──────────┬──────────┘
                                      │
              ┌───────────────────────┼───────────────────────┐
              │                       │                       │
              ▼                       ▼                       ▼
      ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
      │  Inventory    │       │ Thread Manager │       │   Process     │
      │   & Search    │       │ / Worker Pool  │       │   Manager     │
      └───────┬───────┘       └───────┬───────┘       └───────────────┘
              │                       │
              │                 ┌─────┴─────┐
              │                 │ Mutex +   │
              │                 │ Semaphores│
              │                 └─────┬─────┘
              │                       │
              └───────────────┬───────┘
                              ▼
                    ┌───────────────────┐
                    │   Storage Layer   │
                    │ SQLite Repository  │
                    └─────────┬─────────┘
                              │
                              ▼
                         ┌─────────┐
                         │ SQLite  │
                         └─────────┘
```

### Source Organization

```text
BookSelf-orgganizer/
├── CMakeLists.txt
├── include/              # Public/internal headers
├── src/                  # Application and core implementation
├── tests/                # Automated test programs
├── database/             # Database-related resources
├── data/                 # Application/sample data
├── docs/                 # Architecture, algorithms and project documentation
├── logs/                 # Runtime logs
├── build/                # Generated build artifacts and Windows binaries
├── Makefile              # POSIX-oriented convenience build
├── LICENSE
└── README.md
```

---

## Windows Quick Start

### Option 1 — Run the prebuilt executable

1. Clone the repository:

```bash
git clone https://github.com/RifatNSU701/BookSelf-orgganizer.git
cd BookSelf-orgganizer
```

2. Make sure the repository contains the required `data/`, `database/`, and `logs/` directories.

3. Run the application from the repository root:

```bash
./build/BookShelfOrganizer.exe
```

On PowerShell, use:

```powershell
.\build\BookShelfOrganizer.exe
```

Running from the project root is important because the application uses project-relative data and log paths.

### Option 2 — Download only the executable

Use the download button at the top of this README:

**[Download BookShelfOrganizer.exe](https://raw.githubusercontent.com/RifatNSU701/BookSelf-orgganizer/main/build/BookShelfOrganizer.exe)**

Place the executable inside a working copy of the repository and run it from the repository root.

> A future release package should bundle every required runtime dependency and application resource so the program can be copied to a clean Windows machine and launched without a development environment. Until that package is published, the repository-based method is the recommended distribution path.

---

## Build From Source on Windows

### Prerequisites

- Windows 10/11 x64
- Git
- CMake 3.20+
- GCC / MinGW-w64
- Ninja
- SQLite3 development package

The current Windows development environment used for this repository is **MSYS2 UCRT64** with GCC, CMake, Ninja, and SQLite3.

### Configure

From the project root in an MSYS2 UCRT64 terminal:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBSO_BUILD_TESTS=ON
```

### Build

```bash
cmake --build build --parallel
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

A successful test run should report:

```text
100% tests passed out of 2
```

### Run

```bash
./build/BookShelfOrganizer.exe
```

---

## Application Menu

```text
 1. Display all books
 2. Add a book
 3. Search for a book
 4. Sort alphabetically (single-thread)
 5. Sort by genre (single-thread)
 6. Sort by publication year (single-thread)
 7. Organize bookshelf (concurrent)
 8. Display bookshelf
 9. Process demonstration
10. Race-condition demonstration
11. Save inventory
12. Exit
```

---

## How the Concurrent Pipeline Works

The concurrent organization flow is designed to demonstrate synchronization rather than simply create threads for appearance.

```text
                    ┌──────────────────────┐
                    │   Organization Start │
                    └──────────┬───────────┘
                               │
                 ┌─────────────┼─────────────┐
                 ▼             ▼             ▼
             Sort Thread   Sort Thread   Sort Thread
                 │             │             │
                 └─────────────┼─────────────┘
                               │
                        sort_done semaphore
                               │
                               ▼
                     Inventory Update Thread
                               │
                         mutex lock
                               │
                         Update + Shelves
                               │
                        mutex unlock
                               │
                               ▼
                     inventory_done semaphore
                               │
                               ▼
                         Search Thread
```

### Why a mutex?

The inventory is shared mutable state. A mutex prevents simultaneous operations from corrupting or observing partially updated records.

### Why a semaphore?

A semaphore expresses phase completion: the inventory stage must wait until all required sorting workers have completed. A mutex alone cannot represent that dependency cleanly.

---

## Process Demonstration

The process demonstration uses `fork()` and `waitpid()` on environments that provide genuine POSIX process semantics.

### Windows compatibility

Native MinGW/UCRT64 provides the threading and synchronization functionality required by the Windows build, but **genuine POSIX `fork()` is not a native Windows system call**.

For a true `fork()` demonstration, use a POSIX-compatible environment such as:

- Linux
- macOS
- Cygwin
- MSYS2 MSYS runtime

The application does not fake process creation when `fork()` is unavailable.

---

## Data & Persistence

The application uses SQLite for structured persistent storage and also contains project data resources under `data/` and `database/`.

The repository is intentionally designed so that application data and source code are kept separate from generated build artifacts.

Runtime logs are written under:

```text
logs/session.log
```

---

## Testing

The project currently includes two CTest targets:

| Test | Purpose | Status |
|---|---|:---:|
| `repository_persistence` | Validates persistence/repository behavior | ✅ |
| `worker_pool_stress` | Exercises worker-pool concurrency | ✅ |

Latest Windows build verification:

```text
2/2 tests passed
100% tests passed
```

---

## Engineering Notes

The project is intentionally built around a modular core library:

```text
bookshelf_core
```

The main executable and migration utility link against this shared project core, while tests exercise core functionality independently.

Compiler diagnostics are enabled with strict warning flags including:

```text
-Wall
-Wextra
-Wpedantic
-Wshadow
-Wformat=2
```

This makes the build useful not only as an academic demonstration but also as a foundation for continued engineering hardening.

---

## Known Release Considerations

The current repository contains a successfully built Windows executable and passing automated tests. Before publishing this as a polished consumer-facing binary release, the following packaging work is recommended:

- Bundle the required SQLite/runtime DLLs
- Produce a clean `dist/` directory containing only runtime files
- Add a versioned release artifact such as `BookShelfOrganizer-v1.0.0-win64.zip`
- Generate SHA-256 checksums
- Add a GitHub Release with signed/verified artifacts where appropriate
- Add a Windows installer for a one-click installation experience
- Remove generated CMake internals from the user-facing distribution
- Resolve compiler warnings before declaring a zero-warning release
- Replace the deprecated `SQLite::SQLite3` CMake target with `SQLite3::SQLite3`

These items are packaging/release-hardening tasks; they are separate from the successful source build and test verification.

---

## Troubleshooting

### `BookShelfOrganizer.exe` does not start

Run it from the repository root so the relative resource paths resolve correctly:

```powershell
cd C:\Users\ASUS\BookSelf-orgganizer
.\build\BookShelfOrganizer.exe
```

If Windows reports a missing DLL, the current build is relying on a development/runtime dependency that has not yet been bundled. Use the source-build method above or wait for the portable release package.

### SQLite cannot be found during CMake configuration

Install the SQLite development package for your compiler environment and rerun configuration after removing the previous build directory.

### `fork()` is unavailable

This is expected on native Windows builds. Use a genuine POSIX environment if you specifically need the `fork()` demonstration.

### Build directory is stale

Remove it and configure from scratch:

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBSO_BUILD_TESTS=ON
```

---

## Project Documentation

Detailed technical material is available in:

- `docs/architecture.md` — system architecture
- `docs/synchronization.md` — mutexes, semaphores and concurrency
- `docs/algorithms.md` — sorting and organization algorithms
- `docs/testing.md` — testing strategy
- `docs/project_report.md` — project report and technical discussion

---

## License

See [`LICENSE`](./LICENSE) for the project's licensing terms.

---

<div align="center">

### BookShelf Organizer

**Built in C • Engineered around real OS concepts • Designed for continued hardening**

[⬆ Back to top](#bookshelf-organizer)

</div>
