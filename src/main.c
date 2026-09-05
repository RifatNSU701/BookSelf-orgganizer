#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bookshelf_core.h"
#include "storage_sqlite.h"
#include "search.h"
#include "bookshelf.h"
#include "input.h"
#include "logger.h"
#include "synchronization.h"
#include "thread_manager.h"
#include "process_manager.h"
#include "utils.h"

#define DATA_PATH "data/bookshelf.db"
#define LOG_PATH  "logs/session.log"

static void print_banner(void)
{
    printf("\n========================================\n");
    printf("      BOOKSHELF MANAGEMENT SYSTEM\n");
    printf("========================================\n");
}

static void print_menu(void)
{
    printf("\n----------------------------------------\n");
    printf(" 1. Display all books\n");
    printf(" 2. Add a book\n");
    printf(" 3. Search for a book\n");
    printf(" 4. Sort alphabetically\n");
    printf(" 5. Sort by genre\n");
    printf(" 6. Sort by publication date\n");
    printf(" 7. Organize bookshelf\n");
    printf(" 8. Display bookshelf\n");
    printf(" 9. Process demonstration\n");
    printf("10. Race-condition demo\n");
    printf("11. Save inventory\n");
    printf("12. Exit\n");
    printf("----------------------------------------\n");
}

static void action_search(BsInventory *inventory)
{
    BsBook book;
    char query[256];
    int field;
    size_t count = 0;

    printf("\nSearch by:  1) Title   2) Author   3) Genre\n");
    field = input_menu_choice("Enter field: ", 1, 3);
    if (field < 0 || !input_string("Enter search text: ", query, sizeof(query))) return;

    if (bs_inventory_count(inventory, &count) != BS_OK) return;
    for (size_t i = 0; i < count; ++i) {
        if (bs_inventory_get_at(inventory, i, &book) != BS_OK) continue;
        const char *value = field == 1 ? book.title : field == 2 ? book.author : book.genre;
        if (strstr(value, query) != NULL) {
            printf("[%d] %s | %s | %s | %d\n", book.id, book.title, book.author, book.genre, book.year);
        }
    }
}

static void action_add(BsInventory *inventory)
{
    char title[256], author[256], genre[128];
    int year, id;
    if (!input_string("Title : ", title, sizeof(title))) return;
    if (!input_string("Author: ", author, sizeof(author))) return;
    if (!input_string("Genre : ", genre, sizeof(genre))) return;
    if (!input_int("Year  : ", 0, 3000, &year)) return;

    BsError result = bs_inventory_add(inventory, title, author, genre, year, &id);
    if (result == BS_OK) {
        printf("Added book with ID %d.\n", id);
        log_event("Menu", "User added book '%s' (id=%d).", title, id);
    } else {
        printf("Failed to add book: %s\n", bs_error_string(result));
    }
}

static void action_single_sort(BsInventory *inventory, int strategy)
{
    BsError result = strategy == 0 ? bs_inventory_sort_title(inventory) :
                      strategy == 1 ? bs_inventory_sort_genre(inventory) :
                                      bs_inventory_sort_year(inventory);
    if (result == BS_OK) {
        printf("Sorted (single-threaded).\n");
        size_t count = 0;
        BsBook book;
        if (bs_inventory_count(inventory, &count) == BS_OK) {
            for (size_t i = 0; i < count; ++i) {
                if (bs_inventory_get_at(inventory, i, &book) == BS_OK)
                    printf("[%d] %s | %s | %s | %d\n", book.id, book.title, book.author, book.genre, book.year);
            }
        }
    }
}

static void action_organize(BsInventory *inventory, SyncPrimitives *sync)
{
    int strat = input_menu_choice("Choose final order: ", 1, 3);
    if (strat < 0) return;
    BsError result = bs_inventory_assign_shelves(inventory);
    if (result == BS_OK) {
        if (strat == 1) result = bs_inventory_sort_title(inventory);
        else if (strat == 2) result = bs_inventory_sort_genre(inventory);
        else result = bs_inventory_sort_year(inventory);
    }
    (void)sync;
    if (result == BS_OK) printf("Organization succeeded.\n");
    else printf("Organization failed: %s\n", bs_error_string(result));
}

static void action_race_demo(void)
{
    long expected = 4L * 100000L;
    long without_mutex = run_race_demo(0);
    long with_mutex = run_race_demo(1);
    printf("\nRACE CONDITION DEMONSTRATION\n");
    printf("Expected: %ld\n", expected);
    printf("WITHOUT mutex: %ld\n", without_mutex);
    printf("WITH mutex   : %ld\n", with_mutex);
}

int main(void)
{
    BsInventory *inventory = NULL;
    BsSqliteStorage *storage = NULL;
    int running = 1;

    if (logger_init(LOG_PATH) != 0) fprintf(stderr, "Warning: logger could not be initialised.\n");
    log_event("Main", "Program starting.");

    BsError result = bs_inventory_create(&inventory);
    if (result != BS_OK) {
        fprintf(stderr, "Fatal: %s\n", bs_error_string(result));
        logger_close();
        return EXIT_FAILURE;
    }

    result = bs_sqlite_open(&storage, DATA_PATH);
    if (result == BS_OK) result = bs_sqlite_initialize(storage);
    if (result == BS_OK) result = bs_sqlite_load(storage, inventory);
    if (result != BS_OK) {
        fprintf(stderr, "Fatal: database initialization failed: %s\n", bs_error_string(result));
        bs_sqlite_close(storage);
        bs_inventory_destroy(inventory);
        logger_close();
        return EXIT_FAILURE;
    }

    print_banner();
    while (running) {
        int choice = input_menu_choice("Enter choice: ", 1, 12);
        if (choice < 0) break;
        switch (choice) {
            case 1: {
                size_t count = 0; BsBook book;
                if (bs_inventory_count(inventory, &count) == BS_OK)
                    for (size_t i = 0; i < count; ++i)
                        if (bs_inventory_get_at(inventory, i, &book) == BS_OK)
                            printf("[%d] %s | %s | %s | %d\n", book.id, book.title, book.author, book.genre, book.year);
                break;
            }
            case 2: action_add(inventory); break;
            case 3: action_search(inventory); break;
            case 4: action_single_sort(inventory, 0); break;
            case 5: action_single_sort(inventory, 1); break;
            case 6: action_single_sort(inventory, 2); break;
            case 7: {
                SyncPrimitives sync;
                if (sync_init(&sync) == 0) { action_organize(inventory, &sync); sync_destroy(&sync); }
                break;
            }
            case 8: action_single_sort(inventory, 0); break;
            case 9: run_process_demo(DATA_PATH); break;
            case 10: action_race_demo(); break;
            case 11:
                result = bs_sqlite_save(storage, inventory);
                printf(result == BS_OK ? "Inventory saved to %s.\n" : "Failed to save inventory: %s\n", DATA_PATH, bs_error_string(result));
                break;
            case 12: running = 0; break;
        }
    }

    if (bs_sqlite_save(storage, inventory) != BS_OK) fprintf(stderr, "Warning: final database save failed.\n");
    bs_sqlite_close(storage);
    bs_inventory_destroy(inventory);
    logger_close();
    printf("Goodbye.\n");
    return EXIT_SUCCESS;
}
