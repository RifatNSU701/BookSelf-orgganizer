#include "repository.h"

#include <string.h>

BsError bs_repository_open(BsRepository *repository, const char *path)
{
    if (repository == NULL || path == NULL || path[0] == '\0') return BS_INVALID_ARGUMENT;
    memset(repository, 0, sizeof(*repository));

    if (pthread_mutex_init(&repository->lock, NULL) != 0) return BS_SYNC_ERROR;
    repository->initialized = 1;

    BsError result = bs_sqlite_open(&repository->storage, path);
    if (result != BS_OK) {
        pthread_mutex_destroy(&repository->lock);
        repository->initialized = 0;
        return result;
    }

    result = bs_sqlite_initialize(repository->storage);
    if (result != BS_OK) {
        bs_sqlite_close(repository->storage);
        repository->storage = NULL;
        pthread_mutex_destroy(&repository->lock);
        repository->initialized = 0;
    }
    return result;
}

void bs_repository_close(BsRepository *repository)
{
    if (repository == NULL || !repository->initialized) return;
    if (pthread_mutex_lock(&repository->lock) == 0) {
        if (repository->storage != NULL) bs_sqlite_close(repository->storage);
        repository->storage = NULL;
        pthread_mutex_unlock(&repository->lock);
    }
    pthread_mutex_destroy(&repository->lock);
    repository->initialized = 0;
}

BsError bs_repository_load(BsRepository *repository, BsInventory *inventory)
{
    if (repository == NULL || inventory == NULL || !repository->initialized || repository->storage == NULL)
        return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&repository->lock) != 0) return BS_SYNC_ERROR;
    BsError result = bs_sqlite_load(repository->storage, inventory);
    pthread_mutex_unlock(&repository->lock);
    return result;
}

BsError bs_repository_save(BsRepository *repository, const BsInventory *inventory)
{
    if (repository == NULL || inventory == NULL || !repository->initialized || repository->storage == NULL)
        return BS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&repository->lock) != 0) return BS_SYNC_ERROR;
    BsError result = bs_sqlite_save(repository->storage, inventory);
    pthread_mutex_unlock(&repository->lock);
    return result;
}
