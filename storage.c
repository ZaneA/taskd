#include "common.h"
#include "storage.h"

// private

#ifdef DEBUG
int _storage_debug_callback(void *user, int columns, char **column_data, char **column_names)
{
    (void)user;
    (void)column_names;

    for (int i = 0; i < columns; i++) {
        fprintf(stdout, "%s\t", column_data[i]);
    }

    fprintf(stdout, "\n");

    return 0; // Non-zero signals SQLite to abort
}

void _storage_debug_exec_result(storage_t *storage, char *sql)
{
    char *err = NULL;

    assert(storage != NULL);
    assert(storage->conn != NULL);

    // Tasks table
    sqlite3_exec(
        storage->conn,
        sql,
        _storage_debug_callback, // callback
        NULL, // callback user-defined
        &err
    );

    if (err != NULL) {
        fprintf(stderr, "_storage_debug_exec_noresult: %s\n", err);
        sqlite3_free(err);
    }
}

void storage_debug(storage_t *storage)
{
    fprintf(stdout, "\n== plugins ==\n");
    _storage_debug_exec_result(storage, "SELECT * FROM " _PLUGINS_TABLE);
    fprintf(stdout, "\n== variables ==\n");
    _storage_debug_exec_result(storage, "SELECT * FROM " _VARIABLES_TABLE);
    fprintf(stdout, "\n== profiles ==\n");
    _storage_debug_exec_result(storage, "SELECT * FROM " _PROFILES_TABLE);
    fprintf(stdout, "\n== tasks ==\n");
    _storage_debug_exec_result(storage, "SELECT * FROM " _TASKS_TABLE);
    fprintf(stdout, "\n-- Total Queries: %i --\n", storage->queries);
    fprintf(stdout, "\n\n");
}
#endif

// Returns a single column
int _storage_exec_result_callback(void *user, int columns, char **column_data, char **column_names)
{
    (void)columns;
    (void)column_names;

    sprintf(user, "%s", column_data[0]);

    return 0;
}

// public

/**
 * Initialize storage.
 */
int storage_init(storage_t *storage, const char *path)
{
    if (sqlite3_open(path, &storage->conn) == SQLITE_OK) { // _STORAGE_PERSISTENT
        storage_exec_noresult(storage, "ATTACH DATABASE ':memory:' AS " _STORAGE_MEMORY);
        return 0;
    } else {
        fprintf(stderr, "storage_init: %s\n", sqlite3_errmsg(storage->conn));
        exit(1);
    }
}

/**
 * Shutdown storage.
 */
void storage_shutdown()
{
    assert(g_engine.storage.conn != NULL);

    sqlite3_close(g_engine.storage.conn);
}

void storage_exec_result_(storage_t *storage, void *user, int(*callback)(void*, int, char**, char**), const char *format, ...)
{
    va_list args;
    char buffer[BUFSIZ];

    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);

    char *err = NULL;

    assert(storage != NULL);
    assert(storage->conn != NULL);

    storage->queries++;

    // Tasks table
    sqlite3_exec(
        storage->conn,
        buffer,
        callback, // callback
        user, // callback user-defined
        &err
    );

    if (err != NULL) {
        fprintf(stderr, "storage_exec_result_: %s\n", err);
        sqlite3_free(err);
    }
}

/**
 * Exec with result
 */
char* storage_exec_result(storage_t *storage, const char *format, ...)
{
    static char result[256] = {0};
    memset(&result, 0, 256);

    va_list args;
    char buffer[BUFSIZ];

    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);

    storage_exec_result_(storage, &result, _storage_exec_result_callback, "%s", buffer);

    return result;
}

/**
 * Exec with no result
 */
int storage_exec_noresult(storage_t *storage, const char *format, ...)
{
    va_list args;
    char buffer[BUFSIZ];

    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);

    char *err = NULL;

    assert(storage != NULL);
    assert(storage->conn != NULL);

    storage->queries++;

    // Tasks table
    sqlite3_exec(
        storage->conn,
        buffer,
        NULL, // callback
        NULL, // callback user-defined
        &err
    );

    if (err != NULL) {
        fprintf(stderr, "storage_exec_noresult: %s\n", err);
        sqlite3_free(err);
        return 1;
    }

    return 0;
}
