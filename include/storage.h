#ifndef BOOKSHELF_STORAGE_H
#define BOOKSHELF_STORAGE_H

#include "bookshelf_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BsStorage BsStorage;

typedef struct {
    BsError (*open)(BsStorage **storage, const char *location);
    void (*close)(BsStorage *storage);
    BsError (*load)(BsStorage *storage, BsInventory *inventory);
    BsError (*save)(BsStorage *storage, const BsInventory *inventory);
} BsStorageAdapter;

BsError bs_storage_open(BsStorage **storage, const char *location);
void bs_storage_close(BsStorage *storage);
BsError bs_storage_load(BsStorage *storage, BsInventory *inventory);
BsError bs_storage_save(BsStorage *storage, const BsInventory *inventory);

const BsStorageAdapter *bs_storage_file_adapter(void);

#ifdef __cplusplus
}
#endif

#endif
