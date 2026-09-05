#ifndef BOOKSHELF_CORE_H
#define BOOKSHELF_CORE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BS_OK = 0,
    BS_INVALID_ARGUMENT = 1,
    BS_NOT_FOUND = 2,
    BS_ALREADY_EXISTS = 3,
    BS_CAPACITY_EXCEEDED = 4,
    BS_STORAGE_ERROR = 5,
    BS_SYNC_ERROR = 6,
    BS_THREAD_ERROR = 7,
    BS_PARSE_ERROR = 8,
    BS_IO_ERROR = 9
} BsError;

typedef enum { BS_STATUS_AVAILABLE = 0, BS_STATUS_CHECKED_OUT = 1 } BsBookStatus;

typedef struct {
    int id;
    char title[256];
    char author[256];
    char genre[128];
    int year;
    int shelf;
    int position;
    BsBookStatus status;
} BsBook;

typedef struct BsInventory BsInventory;

BsError bs_inventory_create(BsInventory **inventory);
void bs_inventory_destroy(BsInventory *inventory);
BsError bs_inventory_clear(BsInventory *inventory);
BsError bs_inventory_add(BsInventory *inventory, const char *title, const char *author, const char *genre, int year, int *book_id);
BsError bs_inventory_import_book(BsInventory *inventory, const BsBook *book);
BsError bs_inventory_count(const BsInventory *inventory, size_t *count);
BsError bs_inventory_get_at(const BsInventory *inventory, size_t index, BsBook *book);
BsError bs_inventory_get(const BsInventory *inventory, int id, BsBook *book);
BsError bs_inventory_clone(const BsInventory *source, BsInventory **clone);
BsError bs_inventory_assign_shelves(BsInventory *inventory);
BsError bs_inventory_sort_title(BsInventory *inventory);
BsError bs_inventory_sort_genre(BsInventory *inventory);
BsError bs_inventory_sort_year(BsInventory *inventory);
BsError bs_inventory_load_file(BsInventory *inventory, const char *path);
BsError bs_inventory_save_file(const BsInventory *inventory, const char *path);
const char *bs_error_string(BsError error);

#ifdef __cplusplus
}
#endif

#endif
