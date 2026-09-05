#include "storage_sqlite.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

struct BsSqliteStorage {
    sqlite3 *db;
};

static const char *SCHEMA_SQL =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS books ("
    "id INTEGER PRIMARY KEY,"
    "title TEXT NOT NULL,"
    "author TEXT NOT NULL,"
    "genre TEXT NOT NULL,"
    "year INTEGER NOT NULL CHECK (year >= 0 AND year <= 9999),"
    "shelf INTEGER NOT NULL DEFAULT -1,"
    "position INTEGER NOT NULL DEFAULT -1,"
    "status INTEGER NOT NULL DEFAULT 0 CHECK (status IN (0, 1)),"
    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);"
    "CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);"
    "CREATE INDEX IF NOT EXISTS idx_books_genre ON books(genre);"
    "CREATE INDEX IF NOT EXISTS idx_books_year ON books(year);";

BsError bs_sqlite_open(BsSqliteStorage **storage, const char *path)
{
    if (storage == NULL || path == NULL || path[0] == '\0') {
        return BS_INVALID_ARGUMENT;
    }

    BsSqliteStorage *instance = calloc(1, sizeof(*instance));
    if (instance == NULL) {
        return BS_STORAGE_ERROR;
    }

    if (sqlite3_open_v2(path, &instance->db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        sqlite3_close(instance->db);
        free(instance);
        return BS_STORAGE_ERROR;
    }

    *storage = instance;
    return BS_OK;
}

void bs_sqlite_close(BsSqliteStorage *storage)
{
    if (storage == NULL) {
        return;
    }
    sqlite3_close(storage->db);
    free(storage);
}

BsError bs_sqlite_initialize(BsSqliteStorage *storage)
{
    if (storage == NULL || storage->db == NULL) {
        return BS_INVALID_ARGUMENT;
    }

    char *error_message = NULL;
    int result = sqlite3_exec(storage->db, SCHEMA_SQL, NULL, NULL, &error_message);
    sqlite3_free(error_message);
    return result == SQLITE_OK ? BS_OK : BS_STORAGE_ERROR;
}

BsError bs_sqlite_load(BsSqliteStorage *storage, BsInventory *inventory)
{
    if (storage == NULL || storage->db == NULL || inventory == NULL) {
        return BS_INVALID_ARGUMENT;
    }

    const char *sql = "SELECT title, author, genre, year FROM books ORDER BY id";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(storage->db, sql, -1, &statement, NULL) != SQLITE_OK) {
        return BS_STORAGE_ERROR;
    }

    BsError result = BS_OK;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const char *title = (const char *)sqlite3_column_text(statement, 0);
        const char *author = (const char *)sqlite3_column_text(statement, 1);
        const char *genre = (const char *)sqlite3_column_text(statement, 2);
        int year = sqlite3_column_int(statement, 3);
        int book_id = 0;

        if (title == NULL || author == NULL || genre == NULL ||
            bs_inventory_add(inventory, title, author, genre, year, &book_id) != BS_OK) {
            result = BS_STORAGE_ERROR;
            break;
        }
    }

    if (sqlite3_errcode(storage->db) != SQLITE_OK && sqlite3_errcode(storage->db) != SQLITE_DONE) {
        result = BS_STORAGE_ERROR;
    }

    sqlite3_finalize(statement);
    return result;
}

BsError bs_sqlite_save(BsSqliteStorage *storage, const BsInventory *inventory)
{
    if (storage == NULL || storage->db == NULL || inventory == NULL) {
        return BS_INVALID_ARGUMENT;
    }

    if (sqlite3_exec(storage->db, "BEGIN IMMEDIATE TRANSACTION; DELETE FROM books;", NULL, NULL, NULL) != SQLITE_OK) {
        return BS_STORAGE_ERROR;
    }

    const char *sql =
        "INSERT INTO books (id, title, author, genre, year, shelf, position, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(storage->db, sql, -1, &statement, NULL) != SQLITE_OK) {
        sqlite3_exec(storage->db, "ROLLBACK;", NULL, NULL, NULL);
        return BS_STORAGE_ERROR;
    }

    size_t count = 0;
    BsError result = bs_inventory_count(inventory, &count);
    for (size_t i = 0; result == BS_OK && i < count; ++i) {
        BsBook book;
        result = bs_inventory_get(inventory, (int)i + 1, &book);
        if (result != BS_OK) {
            break;
        }

        sqlite3_bind_int(statement, 1, book.id);
        sqlite3_bind_text(statement, 2, book.title, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, book.author, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, book.genre, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 5, book.year);
        sqlite3_bind_int(statement, 6, book.shelf);
        sqlite3_bind_int(statement, 7, book.position);
        sqlite3_bind_int(statement, 8, book.status);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            result = BS_STORAGE_ERROR;
        }
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
    }

    sqlite3_finalize(statement);
    if (result == BS_OK) {
        if (sqlite3_exec(storage->db, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK) {
            result = BS_STORAGE_ERROR;
        }
    } else {
        sqlite3_exec(storage->db, "ROLLBACK;", NULL, NULL, NULL);
    }

    return result;
}
