/*
 * sorting.c
 * Stable insertion sorts over Book arrays. Insertion sort is chosen for its
 * clarity and its STABILITY: when sorting by genre we keep books within the
 * same genre in their existing (title) order, giving a pleasant secondary
 * ordering without extra work.
 *
 * These functions are pure: no locks, no I/O, no globals. That is what lets
 * the worker threads each sort a PRIVATE copy safely (no shared-array race),
 * and lets the inventory thread apply a sort inside its critical section.
 */

#include "sorting.h"
#include "utils.h"

#include <string.h>

/* Generic stable insertion sort driven by a comparison function. */
static void insertion_sort(Book *a, int n,
                           int (*cmp)(const Book *, const Book *))
{
    int i, j;
    for (i = 1; i < n; i++) {
        Book key = a[i];
        j = i - 1;
        /* Move elements greater than key one position ahead. Using '>' (not
         * '>=') preserves stability. */
        while (j >= 0 && cmp(&a[j], &key) > 0) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

static int cmp_title(const Book *x, const Book *y)
{
    return str_casecmp_portable(x->title, y->title);
}

static int cmp_genre(const Book *x, const Book *y)
{
    int c = str_casecmp_portable(x->genre, y->genre);
    if (c != 0) {
        return c;
    }
    /* Secondary key: title, so books within a genre read nicely. */
    return str_casecmp_portable(x->title, y->title);
}

static int cmp_year(const Book *x, const Book *y)
{
    if (x->year != y->year) {
        return (x->year < y->year) ? -1 : 1;
    }
    return str_casecmp_portable(x->title, y->title);
}

void sort_books_by_title(Book *books, int n)
{
    if (books == NULL || n < 2) {
        return;
    }
    insertion_sort(books, n, cmp_title);
}

void sort_books_by_genre(Book *books, int n)
{
    if (books == NULL || n < 2) {
        return;
    }
    insertion_sort(books, n, cmp_genre);
}

void sort_books_by_year(Book *books, int n)
{
    if (books == NULL || n < 2) {
        return;
    }
    insertion_sort(books, n, cmp_year);
}
