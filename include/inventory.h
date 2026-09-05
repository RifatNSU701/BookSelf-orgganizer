#ifndef INVENTORY_H
#define INVENTORY_H

/*
 * inventory.h
 * The shared digital inventory. This is THE shared resource that multiple
 * worker threads read and write concurrently, so every mutating operation is
 * protected by a pthread_mutex_t (see synchronization.c / thread_manager.c).
 */

#include <pthread.h>
#include "utils.h"

#define MAX_BOOKS        128   /* capacity of the in-memory inventory       */
#define SHELF_CAPACITY     8   /* how many books fit on one physical shelf  */
#define MAX_SHELVES       16   /* number of shelves available               */

/* Availability / status of a book on the shelf. */
typedef enum {
    STATUS_AVAILABLE = 0,
    STATUS_CHECKED_OUT = 1
} BookStatus;

/*
 * Book
 * A single record in the inventory. shelf/position are assigned by the
 * bookshelf allocation logic; -1 means "not yet placed".
 */
typedef struct {
    int  id;
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char genre[MAX_GENRE_LEN];
    int  year;
    int  shelf;       /* 1-based shelf number, or -1 if unassigned */
    int  position;    /* 1-based position on the shelf, or -1      */
    BookStatus status;
} Book;

/*
 * Inventory
 * The shared collection plus its guarding mutex. Bundling the mutex with the
 * data it protects is a deliberate design choice: it makes the critical
 * section explicit and keeps the lock discipline in one place.
 */
typedef struct {
    Book            books[MAX_BOOKS];
    int             count;
    pthread_mutex_t lock;   /* protects every field above */
} Inventory;

/* Lifecycle ---------------------------------------------------------------- */
int  inventory_init(Inventory *inv);       /* returns 0 on success */
void inventory_destroy(Inventory *inv);

/* Loading / persistence ---------------------------------------------------- */
int  inventory_load(Inventory *inv, const char *path);   /* returns #loaded, -1 on error */
int  inventory_save(Inventory *inv, const char *path);   /* returns 0 on success */

/*
 * inventory_add
 * Thread-safe add. Acquires the mutex internally. Returns the new book's id,
 * or -1 if the inventory is full / input invalid.
 */
int inventory_add(Inventory *inv, const char *title, const char *author,
                  const char *genre, int year);

/*
 * The following operate on an ALREADY-LOCKED inventory. They are the building
 * blocks used inside critical sections by worker threads. Callers must hold
 * inv->lock. This split keeps lock ownership explicit and avoids double-locking
 * (which would otherwise risk self-deadlock on a non-recursive mutex).
 */
void inventory_sort_by_title_locked(Inventory *inv);
void inventory_sort_by_genre_locked(Inventory *inv);
void inventory_sort_by_year_locked(Inventory *inv);
void inventory_assign_shelves_locked(Inventory *inv);

/* Display helpers (each takes the lock internally) ------------------------- */
void inventory_display_all(Inventory *inv);
void inventory_display_count(Inventory *inv);

#endif /* INVENTORY_H */
