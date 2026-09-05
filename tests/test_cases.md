# Test Cases

Numbered functional test matrix for the Bookshelf Management System. Each case
lists a precondition, the exact input to type at the menu, and the expected
result. Run the program from the project root so `data/books.txt` and
`logs/session.log` resolve correctly.

Legend: inputs are what you type followed by Enter. `^Z` (Windows) or `^D`
(POSIX) signals end-of-file.

---

## TC-01 — Program startup and data load
* **Precondition:** `data/books.txt` present with 20 books.
* **Input:** launch the program.
* **Expected:** prints `Loaded 20 book(s) from data/books.txt.`, shows banner and
  menu. Log file records `Program starting.` and `Loaded 20 book(s).`
* **Result:** PASS

## TC-02 — Missing data file
* **Precondition:** temporarily rename `data/books.txt`.
* **Input:** launch the program.
* **Expected:** prints a warning about the missing file and starts with an empty
  inventory; program does not crash.
* **Result:** PASS

## TC-03 — Display all books
* **Input:** `1`
* **Expected:** a 20-row table with ID, Title, Author, Genre, Year and a
  Shelf/Pos column showing `-` before organizing.
* **Result:** PASS

## TC-04 — Add a valid book
* **Input:** `2` → `Test Driven Development` → `Kent Beck` →
  `Computer Science` → `2002`
* **Expected:** `Added book with ID 21.` message; log line records the add.
  Choosing `1` afterwards shows 21 books.
* **Result:** PASS

## TC-05 — Add with empty title (input validation)
* **Input:** `2` → *(press Enter on empty title)*
* **Expected:** `Input cannot be empty. Please try again.` re-prompt; no empty
  book is stored.
* **Result:** PASS

## TC-06 — Add with invalid year (input validation)
* **Input:** `2` → `Some Title` → `Some Author` → `Some Genre` → `abcd`
* **Expected:** `Invalid number. Please try again.` re-prompt until a valid year
  in range [0, 3000] is entered.
* **Result:** PASS

## TC-07 — Search by title (found)
* **Input:** `3` → `1` (Title) → `Operating`
* **Expected:** one match, showing full details and (if organized)
  `Location: Shelf x, Position y`; `Matches found: 1`.
* **Result:** PASS

## TC-08 — Search by author (multiple matches)
* **Input:** `3` → `2` (Author) → `Tolkien`
* **Expected:** two matches (*The Hobbit*, *The Lord of the Rings*);
  `Matches found: 2`.
* **Result:** PASS

## TC-09 — Search by genre (group)
* **Input:** `3` → `3` (Genre) → `Computer Science`
* **Expected:** all Computer-Science books listed; count matches the data set.
* **Result:** PASS

## TC-10 — Search, no match
* **Input:** `3` → `1` (Title) → `Nonexistent Book Title`
* **Expected:** `No book matched "Nonexistent Book Title".`; `Matches found: 0`;
  no crash.
* **Result:** PASS

## TC-11 — Search field out of range
* **Input:** `3` → `9`
* **Expected:** re-prompt `Please enter a value between 1 and 3.`
* **Result:** PASS

## TC-12 — Single-thread sort by title
* **Input:** `4`
* **Expected:** inventory reordered A→Z by title and displayed.
* **Result:** PASS

## TC-13 — Single-thread sort by genre
* **Input:** `5`
* **Expected:** books grouped by genre, alphabetised within each genre.
* **Result:** PASS

## TC-14 — Single-thread sort by year
* **Input:** `6`
* **Expected:** books ordered oldest→newest; ties broken by title.
* **Result:** PASS

## TC-15 — Concurrent organize (threads + semaphore + mutex)
* **Input:** `7` → choose final order, e.g. `1` (Title)
* **Expected:** log shows 3 sorter threads starting, 3 `sort-completion N of 3`
  lines, then `Inventory lock ACQUIRED`/`RELEASED`, then
  `inventory-done`, then the search thread verifying `20 book(s) placed`.
  Bookshelf is displayed afterwards. Inventory update never starts before all
  3 completions; search never starts before inventory-done.
* **Result:** PASS

## TC-16 — Display bookshelf
* **Input:** `8` *(after organizing)*
* **Expected:** books grouped under `Shelf 1..N`, each `[position] Title
  (Genre, Year)`, up to 8 per shelf.
* **Result:** PASS

## TC-17 — Bookshelf before organizing
* **Input:** `8` *(fresh start, before any organize)*
* **Expected:** all books appear under `Unplaced (no shelf space)` OR the shelf
  is shown empty, since no shelf has been assigned yet.
* **Result:** PASS

## TC-18 — Process demonstration (fork/wait) — POSIX build
* **Precondition:** built under Cygwin/MSYS2/Linux (fork available).
* **Input:** `9`
* **Expected:** parent forks two children with **distinct PIDs**; each child
  logs its pid and parent pid; parent `waitpid`s both and prints their exit
  codes (0). Choosing `1` afterwards confirms the parent's inventory is
  unchanged by the children (address-space isolation).
* **Result:** PASS

## TC-19 — Process demonstration — native MinGW build
* **Precondition:** built with native MinGW (no fork).
* **Input:** `9`
* **Expected:** honest notice that fork() is unavailable on native MinGW and no
  process was created; thread/mutex/semaphore features still work.
* **Result:** PASS (by design; verified via the HAVE_FORK fallback path)

## TC-20 — Race-condition demo
* **Input:** `10`
* **Expected:** `WITHOUT mutex` total is less than 400000 (lost updates);
  `WITH mutex` total is exactly 400000.
* **Result:** PASS

## TC-21 — Save inventory
* **Input:** `11`
* **Expected:** `Inventory saved to data/books.txt.`; file now contains the
  current (possibly reordered / newly added) books in
  `Title|Author|Genre|Year` format and reloads cleanly on next launch.
* **Result:** PASS

## TC-22 — Invalid menu choice (non-numeric)
* **Input:** `hello`
* **Expected:** `Invalid number. Please try again.` re-prompt.
* **Result:** PASS

## TC-23 — Invalid menu choice (out of range)
* **Input:** `99`
* **Expected:** re-prompt `Please enter a value between 1 and 12.`
* **Result:** PASS

## TC-24 — Overly long input line
* **Input:** paste a >512-character string at a prompt.
* **Expected:** buffer is not overflowed; remainder of the line is discarded so
  the next read starts fresh; no crash.
* **Result:** PASS

## TC-25 — EOF handling
* **Input:** `^Z`+Enter (Windows) or `^D` (POSIX) at the menu prompt.
* **Expected:** `EOF received. Exiting.`, clean shutdown, inventory mutex
  destroyed, log closed.
* **Result:** PASS

## TC-26 — Normal exit
* **Input:** `12`
* **Expected:** `Shutting down. Cleaning up resources...` then `Goodbye.`;
  process exits with status 0.
* **Result:** PASS

## TC-27 — Repeated organize runs
* **Input:** `7` … `7` … (organize several times with different final orders)
* **Expected:** each run re-initialises fresh semaphores (counts start at 0),
  completes without hanging, and re-lays the shelf; no resource leak across
  runs (verified under AddressSanitizer).
* **Result:** PASS
