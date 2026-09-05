#ifndef BOOKSHELF_H
#define BOOKSHELF_H

/*
 * bookshelf.h
 * Physical-shelf presentation. Given the (already sorted and shelf-assigned)
 * inventory, print it as a set of shelves and positions. Shelf assignment
 * itself lives in inventory.c (inventory_assign_shelves_locked) because it
 * mutates shared state and must run inside the critical section.
 */

#include "inventory.h"

/* Print the bookshelf grouped by shelf and position. Takes the lock internally. */
void bookshelf_display(Inventory *inv);

#endif /* BOOKSHELF_H */
