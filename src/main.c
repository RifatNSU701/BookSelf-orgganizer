#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bookshelf_core.h"
#include "repository.h"
#include "search.h"
#include "input.h"
#include "logger.h"
#include "thread_manager.h"
#include "process_manager.h"

#define DATA_PATH "data/bookshelf.db"
#define LOG_PATH "logs/session.log"

static void print_banner(void)
{
    printf("\n========================================\n");
    printf("      BOOKSHELF MANAGEMENT SYSTEM\n");
    printf("========================================\n");
}

static void print_menu(void)
{
    printf("\n----------------------------------------\n");
    printf(" 1. Display all books\n2. Add a book\n3. Search for a book\n4. Sort alphabetically\n5. Sort by genre\n6. Sort by publication date\n7. Organize bookshelf\n8. Display bookshelf\n9. Process demonstration\n10. Race-condition demo\n11. Save inventory\n12. Exit\n");
    printf("----------------------------------------\n");
}

static void display_books(const BsInventory *inventory)
{
    size_t count = 0; BsBook book;
    if (bs_inventory_count(inventory, &count) != BS_OK) return;
    printf("\nBooks: %zu\n", count);
    for (size_t i = 0; i < count; ++i)
        if (bs_inventory_get_at(inventory, i, &book) == BS_OK)
            printf("[%d] %s | %s | %s | %d | shelf=%d | position=%d | status=%d\n", book.id, book.title, book.author, book.genre, book.year, book.shelf, book.position, book.status);
}

static void action_search(BsInventory *inventory)
{
    BsBook book; char query[256]; int field; size_t count = 0;
    printf("\nSearch by: 1) Title  2) Author  3) Genre\n");
    field = input_menu_choice("Enter field: ", 1, 3);
    if (field < 0 || !input_string("Enter search text: ", query, sizeof(query))) return;
    if (bs_inventory_count(inventory, &count) != BS_OK) return;
    for (size_t i = 0; i < count; ++i) {
        if (bs_inventory_get_at(inventory, i, &book) != BS_OK) continue;
        const char *value = field == 1 ? book.title : field == 2 ? book.author : book.genre;
        if (strstr(value, query) != NULL) printf("[%d] %s | %s | %s | %d\n", book.id, book.title, book.author, book.genre, book.year);
    }
}

static void action_add(BsInventory *inventory)
{
    char title[256], author[256], genre[128]; int year, id;
    if (!input_string("Title : ", title, sizeof(title))) return;
    if (!input_string("Author: ", author, sizeof(author))) return;
    if (!input_string("Genre : ", genre, sizeof(genre))) return;
    if (!input_int("Year  : ", 0, 3000, &year)) return;
    BsError result = bs_inventory_add(inventory, title, author, genre, year, &id);
    if (result == BS_OK) { printf("Added book with ID %d.\n", id); log_event("Menu", "Added book '%s' (id=%d).", title, id); }
    else printf("Failed to add book: %s\n", bs_error_string(result));
}

static void action_sort(BsInventory *inventory, int strategy)
{
    BsError result = strategy == 0 ? bs_inventory_sort_title(inventory) : strategy == 1 ? bs_inventory_sort_genre(inventory) : bs_inventory_sort_year(inventory);
    if (result == BS_OK) display_books(inventory); else printf("Sort failed: %s\n", bs_error_string(result));
}

static void action_organize(BsInventory *inventory)
{
    int strategy = input_menu_choice("Choose final order: ", 1, 3);
    if (strategy < 0) return;
    if (run_organization_pipeline(inventory, strategy - 1) != 0) printf("Organization failed.\n");
    else { printf("Organization succeeded.\n"); display_books(inventory); }
}

static void action_race_demo(void)
{
    const long expected = 4L * 100000L;
    printf("\nRACE CONDITION DEMONSTRATION\nExpected: %ld\nWITHOUT mutex: %ld\nWITH mutex: %ld\n", expected, run_race_demo(0), run_race_demo(1));
}

int main(void)
{
    BsInventory *inventory = NULL; BsRepository repository = {0}; int running = 1;
    if (logger_init(LOG_PATH) != 0) fprintf(stderr, "Warning: logger could not be initialised.\n");
    log_event("Main", "Program starting.");
    BsError result = bs_inventory_create(&inventory);
    if (result != BS_OK) { fprintf(stderr, "Fatal: %s\n", bs_error_string(result)); logger_close(); return EXIT_FAILURE; }
    result = bs_repository_open(&repository, DATA_PATH);
    if (result == BS_OK) result = bs_repository_load(&repository, inventory);
    if (result != BS_OK) { fprintf(stderr, "Fatal: repository initialization failed: %s\n", bs_error_string(result)); bs_repository_close(&repository); bs_inventory_destroy(inventory); logger_close(); return EXIT_FAILURE; }

    print_banner();
    while (running) {
        print_menu(); int choice = input_menu_choice("Enter choice: ", 1, 12); if (choice < 0) break;
        switch (choice) {
            case 1: case 8: display_books(inventory); break;
            case 2: action_add(inventory); break;
            case 3: action_search(inventory); break;
            case 4: action_sort(inventory, 0); break;
            case 5: action_sort(inventory, 1); break;
            case 6: action_sort(inventory, 2); break;
            case 7: action_organize(inventory); break;
            case 9: run_process_demo(DATA_PATH); break;
            case 10: action_race_demo(); break;
            case 11:
                result = bs_repository_save(&repository, inventory);
                if (result == BS_OK) printf("Inventory saved to %s.\n", DATA_PATH);
                else printf("Failed to save inventory: %s\n", bs_error_string(result));
                break;
            case 12: running = 0; break;
        }
    }
    if (bs_repository_save(&repository, inventory) != BS_OK) fprintf(stderr, "Warning: final database save failed.\n");
    bs_repository_close(&repository); bs_inventory_destroy(inventory); logger_close(); printf("Goodbye.\n"); return EXIT_SUCCESS;
}
