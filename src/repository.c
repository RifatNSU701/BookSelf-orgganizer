#include "repository.h"

#include <string.h>

BsError bs_repository_open(BsRepository *repository, const char *path)
{
    if (repository == NULL || path == NULL || path[0] == '\0') return BS_INVALID_ARGUMENT;
    memset(repository, 0, sizeof(*repository));

    BsError result = bs_sqlite_open(&repository->storage, path);
    if (result != BS_OK) return result;

    result = bs_sqlite_initialize(repository->storage);
    if (result != BS_OK) {
        bs_sqlite_close(repository->storage);
        repository->storage = NULL;
    }
    return result;
}

void bs_repository_close(BsRepository *repository)
{
    if (repository == NULL) return;
    if (repository->storage != NULL) bs_sqlite_close(repository->storage);
    repository->storage = NULL;
}

BsError bs_repository_load(BsRepository *repository, BsInventory *inventory)
{
    if (repository == NULL || repository->storage == NULL || inventory == NULL) return BS_INVALID_ARGUMENT;
    return bs_sqlite_load(repository->storage, inventory);
}

BsError bs_repository_save(BsRepository *repository, const BsInventory *inventory)
{
    if (repository == NULL || repository->storage == NULL || inventory == NULL) return BS_INVALID_ARGUMENT;
    return bs_sqlite_save(repository->storage, inventory);
}
