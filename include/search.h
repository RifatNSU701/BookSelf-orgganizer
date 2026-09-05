#ifndef SEARCH_H
#define SEARCH_H

/*
 * search.h
 * Read operations over the shared inventory. Even reads take the mutex,
 * because a concurrent writer (e.g. the inventory update thread) could
 * otherwise observe a half-updated record. This demonstrates that the
 * critical section covers readers as well as writers.
 */

#include "inventory.h"

/* Search by (case-insensitive substring of) title; prints all matches. */
void search_by_title(Inventory *inv, const char *query);

/* Search by author substring. */
void search_by_author(Inventory *inv, const char *query);

/* Search by genre substring. */
void search_by_genre(Inventory *inv, const char *query);

#endif /* SEARCH_H */
