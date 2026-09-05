#ifndef BOOKSHELF_STORAGE_SQLITE_H
#define BOOKSHELF_STORAGE_SQLITE_H

#include "bookshelf_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BsSqliteStorage BsSqliteStorage;

BsError bs_sqlite_open(BsSqliteStorage **storage, const char *path);
void bs_sqlite_close(BsSqliteStorage *storage);
BsError bs_sqlite_initialize(BsSqliteStorage *storage);
BsError bs_sqlite_load(BsSqliteStorage *storage, BsInventory *inventory);
BsError bs_sqlite_save(BsSqliteStorage *storage, const BsInventory *inventory);

#ifdef __cplusplus
}
#endif

#endif
