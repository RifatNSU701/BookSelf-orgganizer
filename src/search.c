#include "search.h"
#include "utils.h"

#include <stdio.h>

static const char *status_str(BookStatus s)
{
    return (s == STATUS_AVAILABLE) ? "Available" : "Checked Out";
}

static void print_book_detail(const Book *b)
{
    printf("  Title           : %s\n", b->title);
    printf("  Author          : %s\n", b->author);
    printf("  Genre           : %s\n", b->genre);
    printf("  Publication Year: %d\n", b->year);
    if (b->shelf > 0) {
        printf("  Location        : Shelf %d, Position %d\n", b->shelf, b->position);
    } else {
        printf("  Location        : (not yet organized - run 'Organize bookshelf')\n");
    }
    printf("  Status          : %s\n", status_str(b->status));
    printf("  ---------------------------------------------\n");
}

static void search_generic(Inventory *inv, const char *query, int field)
{
    int i, matches = 0;

    if (inv == NULL || query == NULL) {
        return;
    }

    pthread_mutex_lock(&inv->lock);
    printf("\n==================== SEARCH RESULTS ====================\n");
    for (i = 0; i < inv->count; i++) {
        const Book *b = &inv->books[i];
        const char *hay = (field == 0) ? b->title
                        : (field == 1) ? b->author
                                       : b->genre;
        if (str_casestr_portable(hay, query)) {
            print_book_detail(b);
            matches++;
        }
    }
    if (matches == 0) {
        printf("  No book matched \"%s\".\n", query);
        printf("  -------------------------------------------\n");
    }
    printf("Matches found: %d\n", matches);
    printf("=======================================================\n");
    pthread_mutex_unlock(&inv->lock);
}

void search_by_title(Inventory *inv, const char *query)
{
    search_generic(inv, query, 0);
}

void search_by_author(Inventory *inv, const char *query)
{
    search_generic(inv, query, 1);
}

void search_by_genre(Inventory *inv, const char *query)
{
    search_generic(inv, query, 2);
}
