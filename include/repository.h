#ifndef BOOKSHELF_REPOSITORY_H
#define BOOKSHELF_REPOSITORY_H

#include "bookshelf_core.h"
#include "storage_sqlite.h"

typedef struct {
    BsSqliteStorage *storage;
} BsRepository;

BsError bs_repository_open(BsRepository *repository, const char *path);
void bs_repository_close(BsRepository *repository);
BsError bs_repository_load(BsRepository *repository, BsInventory *inventory);
BsError bs_repository_save(BsRepository *repository, const BsInventory *inventory);

#endif
