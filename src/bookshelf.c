/*
 * bookshelf.c
 * Renders the shared inventory as physical shelves. Read-only, but it still
 * takes the mutex so it never prints a shelf mid-way through an update by a
 * concurrent writer thread.
 */

#include "bookshelf.h"

#include <stdio.h>

void bookshelf_display(Inventory *inv)
{
    int shelf, i;
    int any;

    if (inv == NULL) {
        return;
    }

    pthread_mutex_lock(&inv->lock);

    printf("\n==================== PHYSICAL BOOKSHELF ====================\n");

    for (shelf = 1; shelf <= MAX_SHELVES; shelf++) {
        any = 0;
        /* First pass: is anything on this shelf? */
        for (i = 0; i < inv->count; i++) {
            if (inv->books[i].shelf == shelf) {
                any = 1;
                break;
            }
        }
        if (!any) {
            continue;
        }
        printf("\nShelf %d:\n", shelf);
        /* Print in position order (1..SHELF_CAPACITY). */
        int pos;
        for (pos = 1; pos <= SHELF_CAPACITY; pos++) {
            for (i = 0; i < inv->count; i++) {
                Book *b = &inv->books[i];
                if (b->shelf == shelf && b->position == pos) {
                    printf("  [%d] %-34.34s (%s, %d)\n",
                           pos, b->title, b->genre, b->year);
                }
            }
        }
    }

    /* Report any books that could not be placed. */
    any = 0;
    for (i = 0; i < inv->count; i++) {
        if (inv->books[i].shelf <= 0) {
            if (!any) {
                printf("\nUnplaced (no shelf space):\n");
                any = 1;
            }
            printf("  - %s\n", inv->books[i].title);
        }
    }
    if (!any && inv->count == 0) {
        printf("(bookshelf is empty)\n");
    }

    printf("===========================================================\n");

    pthread_mutex_unlock(&inv->lock);
}
