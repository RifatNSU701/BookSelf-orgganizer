#include "bookshelf_core.h"
#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

int main(void)
{
    const char *db_path = "test_bookshelf.db";
    BsInventory *inventory = NULL;
    BsInventory *loaded = NULL;
    BsRepository repository = {0};
    BsBook original = {
        .id = 42,
        .title = "The Professional Book",
        .author = "Test Author",
        .genre = "Engineering",
        .year = 2026,
        .shelf = 3,
        .position = 7,
        .status = BS_STATUS_CHECKED_OUT
    };
    BsBook restored;

    remove(db_path);
    CHECK(bs_inventory_create(&inventory) == BS_OK);
    CHECK(bs_inventory_create(&loaded) == BS_OK);
    CHECK(bs_inventory_import_book(inventory, &original) == BS_OK);
    CHECK(bs_repository_open(&repository, db_path) == BS_OK);
    CHECK(bs_repository_save(&repository, inventory) == BS_OK);
    CHECK(bs_repository_load(&repository, loaded) == BS_OK);
    CHECK(bs_inventory_get(loaded, original.id, &restored) == BS_OK);
    CHECK(restored.id == original.id);
    CHECK(strcmp(restored.title, original.title) == 0);
    CHECK(strcmp(restored.author, original.author) == 0);
    CHECK(strcmp(restored.genre, original.genre) == 0);
    CHECK(restored.year == original.year);
    CHECK(restored.shelf == original.shelf);
    CHECK(restored.position == original.position);
    CHECK(restored.status == original.status);

    bs_repository_close(&repository);
    bs_inventory_destroy(loaded);
    bs_inventory_destroy(inventory);
    remove(db_path);

    if (failures != 0) {
        fprintf(stderr, "%d test assertion(s) failed.\n", failures);
        return EXIT_FAILURE;
    }
    printf("All repository persistence tests passed.\n");
    return EXIT_SUCCESS;
}
