#include "inventory.h"
#include "sorting.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int inventory_init(Inventory *inv)
{
    if (inv == NULL) {
        return -1;
    }
    inv->count = 0;
    memset(inv->books, 0, sizeof(inv->books));
    if (pthread_mutex_init(&inv->lock, NULL) != 0) {
        return -1;
    }
    return 0;
}

void inventory_destroy(Inventory *inv)
{
    if (inv == NULL) {
        return;
    }
    pthread_mutex_destroy(&inv->lock);
}

static int add_book_locked(Inventory *inv, const char *title,
                           const char *author, const char *genre, int year)
{
    Book *b;

    if (inv->count >= MAX_BOOKS) {
        return -1;
    }
    b = &inv->books[inv->count];
    b->id = inv->count + 1;
    safe_strcpy(b->title,  title,  MAX_TITLE_LEN);
    safe_strcpy(b->author, author, MAX_AUTHOR_LEN);
    safe_strcpy(b->genre,  genre,  MAX_GENRE_LEN);
    b->year     = year;
    b->shelf    = -1;
    b->position = -1;
    b->status   = STATUS_AVAILABLE;
    inv->count++;
    return b->id;
}

int inventory_add(Inventory *inv, const char *title, const char *author,
                  const char *genre, int year)
{
    int id;

    if (inv == NULL || title == NULL || author == NULL || genre == NULL) {
        return -1;
    }
    pthread_mutex_lock(&inv->lock);
    id = add_book_locked(inv, title, author, genre, year);
    pthread_mutex_unlock(&inv->lock);
    return id;
}

int inventory_load(Inventory *inv, const char *path)
{
    FILE *fp;
    char  line[MAX_LINE_LEN];
    int   loaded = 0;

    if (inv == NULL || path == NULL) {
        return -1;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    pthread_mutex_lock(&inv->lock);
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *title, *author, *genre, *year_s;
        char *saveptr = NULL;
        int   year;
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        title  = strtok_r(line, "|", &saveptr);
        author = strtok_r(NULL, "|", &saveptr);
        genre  = strtok_r(NULL, "|", &saveptr);
        year_s = strtok_r(NULL, "|", &saveptr);

        if (title == NULL || author == NULL || genre == NULL || year_s == NULL) {
            continue;
        }
        year = atoi(trim(year_s));
        if (add_book_locked(inv, trim(title), trim(author),
                            trim(genre), year) > 0) {
            loaded++;
        }
    }
    pthread_mutex_unlock(&inv->lock);

    fclose(fp);
    return loaded;
}

int inventory_save(Inventory *inv, const char *path)
{
    FILE *fp;
    int   i;

    if (inv == NULL || path == NULL) {
        return -1;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }

    pthread_mutex_lock(&inv->lock);
    fprintf(fp, "# Title|Author|Genre|Year\n");
    for (i = 0; i < inv->count; i++) {
        Book *b = &inv->books[i];
        fprintf(fp, "%s|%s|%s|%d\n", b->title, b->author, b->genre, b->year);
    }
    pthread_mutex_unlock(&inv->lock);

    fclose(fp);
    return 0;
}

void inventory_sort_by_title_locked(Inventory *inv)
{
    sort_books_by_title(inv->books, inv->count);
}

void inventory_sort_by_genre_locked(Inventory *inv)
{
    sort_books_by_genre(inv->books, inv->count);
}

void inventory_sort_by_year_locked(Inventory *inv)
{
    sort_books_by_year(inv->books, inv->count);
}

void inventory_assign_shelves_locked(Inventory *inv)
{
    int i;
    for (i = 0; i < inv->count; i++) {
        int shelf    = i / SHELF_CAPACITY + 1;
        int position = i % SHELF_CAPACITY + 1;
        if (shelf > MAX_SHELVES) {
            inv->books[i].shelf    = -1;
            inv->books[i].position = -1;
        } else {
            inv->books[i].shelf    = shelf;
            inv->books[i].position = position;
        }
    }
}

static const char *status_str(BookStatus s)
{
    return (s == STATUS_AVAILABLE) ? "Available" : "Checked Out";
}

void inventory_display_all(Inventory *inv)
{
    int i;
    if (inv == NULL) {
        return;
    }
    pthread_mutex_lock(&inv->lock);
    printf("\n----------------------------------------------------------------------------------\n");
    printf("%-4s %-34s %-22s %-14s %-6s %-8s\n",
           "ID", "Title", "Author", "Genre", "Year", "Shelf/Pos");
    printf("----------------------------------------------------------------------------------\n");
    for (i = 0; i < inv->count; i++) {
        Book *b = &inv->books[i];
        char loc[16];
        if (b->shelf > 0) {
            snprintf(loc, sizeof(loc), "%d/%d", b->shelf, b->position);
        } else {
            safe_strcpy(loc, "-", sizeof(loc));
        }
        printf("%-4d %-34.34s %-22.22s %-14.14s %-6d %-8s\n",
               b->id, b->title, b->author, b->genre, b->year, loc);
    }
    printf("----------------------------------------------------------------------------------\n");
    printf("Total books: %d   (statuses shown on search)\n", inv->count);
    (void)status_str;
    pthread_mutex_unlock(&inv->lock);
}

void inventory_display_count(Inventory *inv)
{
    int c;
    if (inv == NULL) {
        return;
    }
    pthread_mutex_lock(&inv->lock);
    c = inv->count;
    pthread_mutex_unlock(&inv->lock);
    printf("Inventory currently holds %d book(s).\n", c);
}
