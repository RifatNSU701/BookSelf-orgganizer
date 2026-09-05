#include "sorting.h"
#include "utils.h"

#include <string.h>

static void insertion_sort(Book *a, int n,
                           int (*cmp)(const Book *, const Book *))
{
    int i, j;
    for (i = 1; i < n; i++) {
        Book key = a[i];
        j = i - 1;
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
