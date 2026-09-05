#include "bookshelf_core.h"
#include "inventory.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct BsInventory {
    Inventory inventory;
};

static void copy_book(BsBook *destination, const Book *source)
{
    destination->id = source->id;
    strncpy(destination->title, source->title, sizeof(destination->title) - 1);
    destination->title[sizeof(destination->title) - 1] = '\0';
    strncpy(destination->author, source->author, sizeof(destination->author) - 1);
    destination->author[sizeof(destination->author) - 1] = '\0';
    strncpy(destination->genre, source->genre, sizeof(destination->genre) - 1);
    destination->genre[sizeof(destination->genre) - 1] = '\0';
    destination->year = source->year;
    destination->shelf = source->shelf;
    destination->position = source->position;
    destination->status = (BsBookStatus)source->status;
}

BsError bs_inventory_create(BsInventory **inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    BsInventory *instance = calloc(1, sizeof(*instance));
    if (instance == NULL) return BS_STORAGE_ERROR;
    if (inventory_init(&instance->inventory) != 0) {
        free(instance);
        return BS_SYNC_ERROR;
    }
    *inventory = instance;
    return BS_OK;
}

void bs_inventory_destroy(BsInventory *inventory)
{
    if (inventory == NULL) return;
    inventory_destroy(&inventory->inventory);
    free(inventory);
}

BsError bs_inventory_clear(BsInventory *inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    inventory->inventory.count = 0;
    memset(inventory->inventory.books, 0, sizeof(inventory->inventory.books));
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_add(BsInventory *inventory, const char *title, const char *author, const char *genre, int year, int *book_id)
{
    if (inventory == NULL || title == NULL || author == NULL || genre == NULL || book_id == NULL) return BS_INVALID_ARGUMENT;
    int id = inventory_add(&inventory->inventory, title, author, genre, year);
    if (id < 0) return BS_CAPACITY_EXCEEDED;
    *book_id = id;
    return BS_OK;
}

BsError bs_inventory_import_book(BsInventory *inventory, const BsBook *book)
{
    if (inventory == NULL || book == NULL || book->id <= 0) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    if (inventory->inventory.count >= MAX_BOOKS) {
        pthread_mutex_unlock(&inventory->inventory.lock);
        return BS_CAPACITY_EXCEEDED;
    }
    for (int i = 0; i < inventory->inventory.count; ++i) {
        if (inventory->inventory.books[i].id == book->id) {
            pthread_mutex_unlock(&inventory->inventory.lock);
            return BS_ALREADY_EXISTS;
        }
    }
    Book *target = &inventory->inventory.books[inventory->inventory.count++];
    target->id = book->id;
    strncpy(target->title, book->title, sizeof(target->title) - 1);
    target->title[sizeof(target->title) - 1] = '\0';
    strncpy(target->author, book->author, sizeof(target->author) - 1);
    target->author[sizeof(target->author) - 1] = '\0';
    strncpy(target->genre, book->genre, sizeof(target->genre) - 1);
    target->genre[sizeof(target->genre) - 1] = '\0';
    target->year = book->year;
    target->shelf = book->shelf;
    target->position = book->position;
    target->status = (BookStatus)book->status;
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_count(const BsInventory *inventory, size_t *count)
{
    if (inventory == NULL || count == NULL) return BS_INVALID_ARGUMENT;
    Inventory *mutable_inventory = (Inventory *)&inventory->inventory;
    if (pthread_mutex_lock(&mutable_inventory->lock) != 0) return BS_SYNC_ERROR;
    *count = (size_t)mutable_inventory->count;
    pthread_mutex_unlock(&mutable_inventory->lock);
    return BS_OK;
}

BsError bs_inventory_get_at(const BsInventory *inventory, size_t index, BsBook *book)
{
    if (inventory == NULL || book == NULL) return BS_INVALID_ARGUMENT;
    Inventory *mutable_inventory = (Inventory *)&inventory->inventory;
    if (pthread_mutex_lock(&mutable_inventory->lock) != 0) return BS_SYNC_ERROR;
    if (index >= (size_t)mutable_inventory->count) {
        pthread_mutex_unlock(&mutable_inventory->lock);
        return BS_NOT_FOUND;
    }
    copy_book(book, &mutable_inventory->books[index]);
    pthread_mutex_unlock(&mutable_inventory->lock);
    return BS_OK;
}

BsError bs_inventory_get(const BsInventory *inventory, int id, BsBook *book)
{
    if (inventory == NULL || book == NULL || id <= 0) return BS_INVALID_ARGUMENT;
    Inventory *mutable_inventory = (Inventory *)&inventory->inventory;
    if (pthread_mutex_lock(&mutable_inventory->lock) != 0) return BS_SYNC_ERROR;
    for (int i = 0; i < mutable_inventory->count; ++i) {
        if (mutable_inventory->books[i].id == id) {
            copy_book(book, &mutable_inventory->books[i]);
            pthread_mutex_unlock(&mutable_inventory->lock);
            return BS_OK;
        }
    }
    pthread_mutex_unlock(&mutable_inventory->lock);
    return BS_NOT_FOUND;
}

BsError bs_inventory_assign_shelves(BsInventory *inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    inventory_assign_shelves_locked(&inventory->inventory);
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_sort_title(BsInventory *inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    inventory_sort_by_title_locked(&inventory->inventory);
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_sort_genre(BsInventory *inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    inventory_sort_by_genre_locked(&inventory->inventory);
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_sort_year(BsInventory *inventory)
{
    if (inventory == NULL) return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&inventory->inventory.lock) != 0) return BS_SYNC_ERROR;
    inventory_sort_by_year_locked(&inventory->inventory);
    pthread_mutex_unlock(&inventory->inventory.lock);
    return BS_OK;
}

BsError bs_inventory_load_file(BsInventory *inventory, const char *path)
{
    if (inventory == NULL || path == NULL) return BS_INVALID_ARGUMENT;
    return inventory_load(&inventory->inventory, path) >= 0 ? BS_OK : BS_IO_ERROR;
}

BsError bs_inventory_save_file(const BsInventory *inventory, const char *path)
{
    if (inventory == NULL || path == NULL) return BS_INVALID_ARGUMENT;
    return inventory_save((Inventory *)&inventory->inventory, path) == 0 ? BS_OK : BS_IO_ERROR;
}

const char *bs_error_string(BsError error)
{
    switch (error) {
    case BS_OK: return "Success";
    case BS_INVALID_ARGUMENT: return "Invalid argument";
    case BS_NOT_FOUND: return "Not found";
    case BS_ALREADY_EXISTS: return "Already exists";
    case BS_CAPACITY_EXCEEDED: return "Capacity exceeded";
    case BS_STORAGE_ERROR: return "Storage error";
    case BS_SYNC_ERROR: return "Synchronization error";
    case BS_THREAD_ERROR: return "Thread error";
    case BS_PARSE_ERROR: return "Parse error";
    case BS_IO_ERROR: return "I/O error";
    default: return "Unknown error";
    }
}
