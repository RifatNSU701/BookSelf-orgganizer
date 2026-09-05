#include "storage.h"

#include "inventory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct BsStorage {
    char *location;
};

static BsError file_open(BsStorage **storage, const char *location)
{
    if (storage == NULL || location == NULL || location[0] == '\0') {
        return BS_INVALID_ARGUMENT;
    }

    BsStorage *instance = calloc(1, sizeof(*instance));
    if (instance == NULL) {
        return BS_STORAGE_ERROR;
    }

    size_t length = strlen(location);
    instance->location = malloc(length + 1);
    if (instance->location == NULL) {
        free(instance);
        return BS_STORAGE_ERROR;
    }

    memcpy(instance->location, location, length + 1);
    *storage = instance;
    return BS_OK;
}

static void file_close(BsStorage *storage)
{
    if (storage == NULL) {
        return;
    }
    free(storage->location);
    free(storage);
}

static BsError file_load(BsStorage *storage, BsInventory *inventory)
{
    if (storage == NULL || inventory == NULL) {
        return BS_INVALID_ARGUMENT;
    }
    int loaded = inventory_load(&inventory->inventory, storage->location);
    return loaded >= 0 ? BS_OK : BS_IO_ERROR;
}

static BsError file_save(BsStorage *storage, const BsInventory *inventory)
{
    if (storage == NULL || inventory == NULL) {
        return BS_INVALID_ARGUMENT;
    }
    int result = inventory_save((Inventory *)&inventory->inventory, storage->location);
    return result == 0 ? BS_OK : BS_IO_ERROR;
}

static const BsStorageAdapter FILE_ADAPTER = {
    .open = file_open,
    .close = file_close,
    .load = file_load,
    .save = file_save
};

const BsStorageAdapter *bs_storage_file_adapter(void)
{
    return &FILE_ADAPTER;
}

BsError bs_storage_open(BsStorage **storage, const char *location)
{
    return file_open(storage, location);
}

void bs_storage_close(BsStorage *storage)
{
    file_close(storage);
}

BsError bs_storage_load(BsStorage *storage, BsInventory *inventory)
{
    return file_load(storage, inventory);
}

BsError bs_storage_save(BsStorage *storage, const BsInventory *inventory)
{
    return file_save(storage, inventory);
}
