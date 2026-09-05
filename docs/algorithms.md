# Algorithms

This document describes the sorting and placement algorithms used by the
Bookshelf Management System and explains why each was chosen. All sorting code
lives in `src/sorting.c`; shelf placement lives in `src/inventory.c`
(`inventory_assign_shelves_locked`).

---

## 1. Why insertion sort?

Every sort in the project is a **stable insertion sort** driven by a
comparison-function pointer (`insertion_sort` in `src/sorting.c`). The choice is
deliberate for an OS course project:

* **Clarity.** The algorithm is short and easy to explain at a viva. The focus
  of the project is *synchronization*, not asymptotic sorting performance, so a
  clear O(n^2) sort over ~20 books is entirely appropriate (20^2 = 400
  comparisons worst case — negligible).
* **Stability.** Insertion sort preserves the relative order of records that
  compare equal. This gives a pleasant *secondary* ordering for free: when we
  group by genre, books inside a genre stay in title order.
* **In-place.** It needs no extra array, so a worker thread can sort its private
  copy without further allocation.
* **Purity.** The routines take a `Book *` and a count only — no locks, no I/O,
  no globals. That purity is exactly what makes them safe to call both from a
  single-threaded menu action *and* from inside a locked critical section in the
  inventory thread, and what lets three worker threads each sort a private copy
  with no shared state.

### Pseudocode

```
insertion_sort(a, n, cmp):
    for i from 1 to n-1:
        key = a[i]
        j = i - 1
        while j >= 0 and cmp(a[j], key) > 0:   # '>' (not '>=') keeps it stable
            a[j+1] = a[j]
            j = j - 1
        a[j+1] = key
```

The strict `>` comparison is what makes the sort stable: an element equal to
`key` is never shifted past it, so equal elements keep their original order.

---

## 2. The three comparison functions

The same engine is reused with three comparators (all case-insensitive via
`str_casecmp_portable` in `src/utils.c`, so ordering is human-friendly and
platform-independent):

### 2.1 By title — `cmp_title`

```
cmp_title(x, y) = case_insensitive_compare(x.title, y.title)
```

Primary and only key: the book title. Produces a strict A→Z ordering.

### 2.2 By genre — `cmp_genre`

```
cmp_genre(x, y):
    c = case_insensitive_compare(x.genre, y.genre)
    if c != 0: return c
    return case_insensitive_compare(x.title, y.title)   # secondary key
```

Groups all books of one genre together, and within each genre orders them by
title. This is where stability + an explicit secondary key gives a clean,
readable grouping (e.g. all *Computer Science* books together, alphabetised).

### 2.3 By publication year — `cmp_year`

```
cmp_year(x, y):
    if x.year != y.year: return (x.year < y.year) ? -1 : +1
    return case_insensitive_compare(x.title, y.title)   # secondary key
```

Orders oldest→newest; ties (same year) fall back to title order.

---

## 3. Case-insensitive comparison — `str_casecmp_portable`

Rather than depend on the non-standard `strcasecmp` (POSIX) / `stricmp`
(Windows), the project ships its own portable implementation that lowercases
each character with `tolower` before comparing. This guarantees **identical
ordering on MinGW and on Cygwin/Linux**, which matters because the same data
file is sorted on both platforms.

Substring search (`str_casestr_portable`) is implemented the same way and backs
the Search feature, so users can type queries in any case.

---

## 4. Shelf-placement algorithm

After the final ordering is chosen, `inventory_assign_shelves_locked` maps the
linear, sorted list onto a 2-D grid of shelves and positions:

```
for i from 0 to count-1:
    shelf    = i / SHELF_CAPACITY + 1     # 1-based shelf number
    position = i % SHELF_CAPACITY + 1     # 1-based slot on that shelf
    if shelf > MAX_SHELVES:
        mark book as unplaced (shelf = -1, position = -1)
    else:
        book.shelf = shelf; book.position = position
```

With `SHELF_CAPACITY = 8` and `MAX_SHELVES = 16` the shelf can hold up to 128
books (equal to `MAX_BOOKS`), so the sample set of 20 always fits. Books beyond
capacity are explicitly marked *unplaced* rather than being allowed to overflow
the grid — a small but important bounds check.

This function **mutates shared state**, so by contract it must run with
`inv->lock` held. It is only ever called from the inventory thread's critical
section (see `docs/synchronization.md`).

---

## 5. Complexity summary

| Operation                    | Time            | Space | Notes                              |
|------------------------------|-----------------|-------|------------------------------------|
| Insertion sort (each sorter) | O(n^2) worst    | O(1)  | n ≈ 20; runs on a private copy     |
| Private-copy snapshot        | O(n)            | O(n)  | one `malloc` + `memcpy` per sorter |
| Shelf assignment             | O(n)            | O(1)  | single pass                        |
| Search (title/author/genre)  | O(n·m)          | O(1)  | m = query length; linear scan      |
| Load / save                  | O(n·L)          | O(1)  | L = line length                    |

For the project's scale these are all effectively instant; the algorithms were
chosen for readability and correctness under concurrency, not raw speed.
