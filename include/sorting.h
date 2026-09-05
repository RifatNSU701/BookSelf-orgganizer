#ifndef SORTING_H
#define SORTING_H

/*
 * sorting.h
 * Pure, reusable sorting routines over arrays of Book. These are deliberately
 * side-effect free (no locking, no I/O) so they can be:
 *   - called from single-threaded menu actions, and
 *   - called from inside a locked critical section by worker threads.
 *
 * Each routine sorts a caller-owned array in place using a stable insertion
 * sort (chosen for clarity and stability, which matters when grouping by
 * genre while keeping a secondary title order).
 */

#include "inventory.h"

void sort_books_by_title(Book *books, int n);
void sort_books_by_genre(Book *books, int n);
void sort_books_by_year(Book *books, int n);

#endif /* SORTING_H */
