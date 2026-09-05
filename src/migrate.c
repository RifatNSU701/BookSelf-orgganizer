#include "bookshelf_core.h"
#include "storage_sqlite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s [--dry-run] <input.txt> <output.db>\n", program);
}

static int parse_book_line(char *line, BsBook *book, int generated_id)
{
    char *saveptr = NULL;
    char *title = strtok_r(line, "|", &saveptr);
    char *author = strtok_r(NULL, "|", &saveptr);
    char *genre = strtok_r(NULL, "|", &saveptr);
    char *year_text = strtok_r(NULL, "|", &saveptr);
    char *end = NULL;
    long year;

    if (title == NULL || author == NULL || genre == NULL || year_text == NULL) return 0;
    year = strtol(year_text, &end, 10);
    if (end == year_text || *end != '\0' || year < 0 || year > 9999) return 0;

    memset(book, 0, sizeof(*book));
    book->id = generated_id;
    snprintf(book->title, sizeof(book->title), "%s", title);
    snprintf(book->author, sizeof(book->author), "%s", author);
    snprintf(book->genre, sizeof(book->genre), "%s", genre);
    book->year = (int)year;
    book->shelf = -1;
    book->position = -1;
    book->status = BS_STATUS_AVAILABLE;
    return 1;
}

int main(int argc, char **argv)
{
    int dry_run = 0;
    const char *input_path;
    const char *output_path;
    FILE *input;
    char line[1024];
    int next_id = 1;
    size_t imported = 0;
    size_t skipped = 0;
    BsInventory *inventory = NULL;
    BsSqliteStorage *storage = NULL;
    BsError result;

    if (argc == 4 && strcmp(argv[1], "--dry-run") == 0) {
        dry_run = 1;
        input_path = argv[2];
        output_path = argv[3];
    } else if (argc == 3) {
        input_path = argv[1];
        output_path = argv[2];
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    input = fopen(input_path, "r");
    if (input == NULL) {
        fprintf(stderr, "Cannot open input file: %s\n", input_path);
        return EXIT_FAILURE;
    }

    result = bs_inventory_create(&inventory);
    if (result != BS_OK) {
        fclose(input);
        fprintf(stderr, "Cannot create inventory: %s\n", bs_error_string(result));
        return EXIT_FAILURE;
    }

    while (fgets(line, sizeof(line), input) != NULL) {
        size_t length = strlen(line);
        BsBook book;
        char *newline;

        if (length == 0) continue;
        newline = strchr(line, '\n');
        if (newline != NULL) *newline = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;

        if (!parse_book_line(line, &book, next_id)) {
            ++skipped;
            continue;
        }

        result = bs_inventory_import_book(inventory, &book);
        if (result != BS_OK) {
            ++skipped;
            continue;
        }
        ++imported;
        ++next_id;
    }
    fclose(input);

    printf("Migration report\n");
    printf("  Input : %s\n", input_path);
    printf("  Output: %s\n", output_path);
    printf("  Valid : %zu\n", imported);
    printf("  Skipped: %zu\n", skipped);

    if (dry_run) {
        printf("Dry run complete. No database was modified.\n");
        bs_inventory_destroy(inventory);
        return skipped == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    result = bs_sqlite_open(&storage, output_path);
    if (result == BS_OK) result = bs_sqlite_initialize(storage);
    if (result == BS_OK) result = bs_sqlite_save(storage, inventory);

    if (result != BS_OK) {
        fprintf(stderr, "Migration failed: %s\n", bs_error_string(result));
        bs_sqlite_close(storage);
        bs_inventory_destroy(inventory);
        return EXIT_FAILURE;
    }

    printf("Migration completed successfully.\n");
    bs_sqlite_close(storage);
    bs_inventory_destroy(inventory);
    return skipped == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
