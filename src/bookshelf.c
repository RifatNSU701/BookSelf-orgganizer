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
