#ifndef SEARCH_H
#define SEARCH_H

#include "inventory.h"

void search_by_title(Inventory *inv, const char *query);
void search_by_author(Inventory *inv, const char *query);
void search_by_genre(Inventory *inv, const char *query);

#endif
