#include "common.h"
#include "variables.h"

// private


// public

/**
 * Initialize variable store.
 */
int variables_init(variables_t *variables)
{
    assert(variables != NULL);

    // Variables table
    storage_exec_noresult(
        &g_engine.storage,
        "CREATE TABLE IF NOT EXISTS " _VARIABLES_TABLE
        "("
            "key VARCHAR(48) PRIMARY KEY,"
            "value TEXT,"
            "description TEXT"
        ");"
    );

    return 0;
}

/**
 * Shutdown variable store.
 */
void variables_shutdown()
{
    // Maybe drop variables table
}

/**
 * Retrieve a variable by key.
 */
char* variables_get(const char *key)
{
    return storage_exec_result(&g_engine.storage, "SELECT value FROM " _VARIABLES_TABLE " WHERE key = \"%s\" LIMIT 1", key);
}

void variables_set(const char *key, const char *format, ...)
{
    va_list args;
    char buffer[BUFSIZ];

    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);

    storage_exec_noresult(
        &g_engine.storage,
        "INSERT OR REPLACE INTO " _VARIABLES_TABLE
        "("
            "key, value, description"
        ") VALUES ("
            "\"%s\", \"%s\", (SELECT description FROM " _VARIABLES_TABLE " WHERE key = \"%s\")"
        ")",
        key,
        buffer,
        key
    );

    // @todo this should queue tasks instead of running them inline
    profiles_queue(&g_engine.profiles, PROFILES_CONDITION_VARIABLE_CHANGED, key);
}
