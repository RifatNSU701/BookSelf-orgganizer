#ifndef INVENTORY_H
#define INVENTORY_H

#include <pthread.h>
#include "utils.h"

#define MAX_BOOKS 128
#define SHELF_CAPACITY 8
#define MAX_SHELVES 16

typedef enum {
    STATUS_AVAILABLE = 0,
    STATUS_CHECKED_OUT = 1
} BookStatus;

typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char genre[MAX_GENRE_LEN];
    int year;
    int shelf;
    int position;
    BookStatus status;
} Book;

typedef struct {
    Book books[MAX_BOOKS];
    int count;
    pthread_mutex_t lock;
} Inventory;

int inventory_init(Inventory *inv);
void inventory_destroy(Inventory *inv);
int inventory_clear(Inventory *inv);
int inventory_import_book(Inventory *inv, const Book *book);
int inventory_load(Inventory *inv, const char *path);
int inventory_save(Inventory *inv, const char *path);
int inventory_add(Inventory *inv, const char *title, const char *author,
                  const char *genre, int year);
void inventory_sort_by_title_locked(Inventory *inv);
void inventory_sort_by_genre_locked(Inventory *inv);
void inventory_sort_by_year_locked(Inventory *inv);
void inventory_assign_shelves_locked(Inventory *inv);
void inventory_display_all(Inventory *inv);
void inventory_display_count(Inventory *inv);

#endif
