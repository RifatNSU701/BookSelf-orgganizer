#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inventory.h"
#include "search.h"
#include "bookshelf.h"
#include "input.h"
#include "logger.h"
#include "synchronization.h"
#include "thread_manager.h"
#include "process_manager.h"
#include "utils.h"

#define DATA_PATH "data/books.txt"
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

static void action_search(Inventory *inv)
{
    int field;
    char query[MAX_TITLE_LEN];

    printf("\nSearch by:  1) Title   2) Author   3) Genre\n");
    field = input_menu_choice("Enter field: ", 1, 3);
    if (field < 0) {
        return;
    }
    if (!input_string("Enter search text: ", query, sizeof(query))) {
        return;
    }
    switch (field) {
        case 1: search_by_title(inv, query);  break;
        case 2: search_by_author(inv, query); break;
        default: search_by_genre(inv, query); break;
    }
}

static void action_add(Inventory *inv)
{
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char genre[MAX_GENRE_LEN];
    int  year, id;

    if (!input_string("Title : ", title, sizeof(title)))  return;
    if (!input_string("Author: ", author, sizeof(author))) return;
    if (!input_string("Genre : ", genre, sizeof(genre)))   return;
    if (!input_int   ("Year  : ", 0, 3000, &year))         return;

    id = inventory_add(inv, title, author, genre, year);
    if (id > 0) {
        printf("Added book with ID %d. (Run 'Organize bookshelf' to place it.)\n", id);
        log_event("Menu", "User added book '%s' (id=%d).", title, id);
    } else {
        printf("Failed to add book (inventory full?).\n");
    }
}

static void action_single_sort(Inventory *inv, int strategy)
{
    pthread_mutex_lock(&inv->lock);
    switch (strategy) {
        case 0: inventory_sort_by_title_locked(inv); break;
        case 1: inventory_sort_by_genre_locked(inv); break;
        default: inventory_sort_by_year_locked(inv); break;
    }
    pthread_mutex_unlock(&inv->lock);
    printf("Sorted (single-threaded).\n");
    inventory_display_all(inv);
}

static void action_organize(Inventory *inv, SyncPrimitives *sync)
{
    int strat;
    printf("\nFinal shelf order:  1) Title   2) Genre   3) Year\n");
    strat = input_menu_choice("Choose final order: ", 1, 3);
    if (strat < 0) {
        return;
    }
    if (run_organization_pipeline(inv, sync, strat - 1) == 0) {
        printf("\nOrganization succeeded. Showing bookshelf:\n");
        bookshelf_display(inv);
    } else {
        printf("Organization failed (thread creation error).\n");
    }
}

static void action_race_demo(void)
{
    long expected = 4L * 100000L;
    long without_mutex, with_mutex;

    printf("\n========================================\n");
    printf(" RACE CONDITION DEMONSTRATION\n");
    printf("========================================\n");
    printf("4 threads each increment a shared counter 100000 times.\n");
    printf("Expected correct total: %ld\n\n", expected);

    without_mutex = run_race_demo(0);
    printf("WITHOUT mutex : %ld  %s\n", without_mutex,
           (without_mutex == expected) ? "(happened to be correct this run)"
                                       : "(WRONG - lost updates due to race!)");

    with_mutex = run_race_demo(1);
    printf("WITH mutex    : %ld  %s\n", with_mutex,
           (with_mutex == expected) ? "(correct - mutual exclusion enforced)"
                                    : "(unexpected)");
    printf("========================================\n");
    printf("The unprotected run races on a read-modify-write; the mutex makes\n");
    printf("the increment a critical section, so no updates are lost.\n");
}

int main(void)
{
    Inventory      inventory;
    SyncPrimitives sync;
    int            loaded;
    int            running = 1;

    if (logger_init(LOG_PATH) != 0) {
        fprintf(stderr, "Warning: logger could not be initialised.\n");
    }
    log_event("Main", "Program starting.");

    if (inventory_init(&inventory) != 0) {
        fprintf(stderr, "Fatal: could not initialise inventory mutex.\n");
        logger_close();
        return EXIT_FAILURE;
    }

    loaded = inventory_load(&inventory, DATA_PATH);
    if (loaded < 0) {
        printf("Warning: could not open %s. Starting with an empty inventory.\n",
               DATA_PATH);
        log_event("Main", "Data file missing; empty inventory.");
    } else {
        printf("Loaded %d book(s) from %s.\n", loaded, DATA_PATH);
        log_event("Main", "Loaded %d book(s).", loaded);
    }

    print_banner();

    while (running) {
        int choice;
        print_menu();
        choice = input_menu_choice("Enter choice: ", 1, 12);
        if (choice < 0) {
            printf("\nEOF received. Exiting.\n");
            break;
        }
        switch (choice) {
            case 1:  inventory_display_all(&inventory); break;
            case 2:  action_add(&inventory); break;
            case 3:  action_search(&inventory); break;
            case 4:  action_single_sort(&inventory, 0); break;
            case 5:  action_single_sort(&inventory, 1); break;
            case 6:  action_single_sort(&inventory, 2); break;
            case 7:
                if (sync_init(&sync) != 0) {
                    printf("Failed to initialise semaphores.\n");
                    break;
                }
                action_organize(&inventory, &sync);
                sync_destroy(&sync);
                break;
            case 8:  bookshelf_display(&inventory); break;
            case 9:  run_process_demo(DATA_PATH); break;
            case 10: action_race_demo(); break;
            case 11:
                if (inventory_save(&inventory, DATA_PATH) == 0) {
                    printf("Inventory saved to %s.\n", DATA_PATH);
                    log_event("Main", "Inventory saved.");
                } else {
                    printf("Failed to save inventory.\n");
                }
                break;
            case 12:
                running = 0;
                break;
            default:
                printf("Unknown choice.\n");
                break;
        }
    }

    printf("\nShutting down. Cleaning up resources...\n");
    log_event("Main", "Shutting down: destroying inventory mutex.");
    inventory_destroy(&inventory);
    logger_close();
    printf("Goodbye.\n");
    return EXIT_SUCCESS;
}
