#include "common.h"
#include "profiles.h"

// private


// public

/**
 * Initialize profile store.
 */
int profiles_init(profiles_t *profiles)
{
    assert(profiles != NULL);

    // Profiles table
    storage_exec_noresult(
        &g_engine.storage,
        "CREATE TABLE IF NOT EXISTS " _PROFILES_TABLE
        "("
            "id INTEGER PRIMARY KEY,"
            "name VARCHAR(50),"
            "enter_task VARCHAR(50),"
            "tick_task VARCHAR(50),"
            "exit_task VARCHAR(50),"
            "condition_type INTEGER,"
            "condition_custom TEXT,"
            "tick_rate INTEGER"
        ");"
        "CREATE TABLE IF NOT EXISTS " _PROFILES_INTERNAL_TABLE
        "("
            "profile_id INTEGER PRIMARY KEY,"
            "state INTEGER,"
            "last_run INTEGER"
        ");"
    );

    return 0;
}

/**
 * Shutdown profile store.
 */
void profiles_shutdown()
{
}

/**
 * Handle profile task triggering logic.
 * Debounces and handles enter/exit tasks.
 */
int _profiles_run_callback(void *user, int columns, char **column_data, char **column_names)
{
    (void)columns;
    (void)column_names;

    int condition_type = (intptr_t)user;

    int state = column_data[7] ? atoi(column_data[7]) : PROFILES_STATE_INACTIVE;
    long last_run = column_data[8] ? atoi(column_data[8]) : 0;
    long tick_rate = (column_data[9] ? atoi(column_data[9]) : 1000);

    // Lower engine tick rate if necessary
    if (tick_rate > 0 && tick_rate < g_engine.tick_rate) {
        g_engine.tick_rate = tick_rate;
    }

    // Skip task if it was recently run
    if (last_run + tick_rate > g_engine.now) {
        return 0;
    }

    switch (condition_type) {
        case PROFILES_CONDITION_ALWAYS:
        case PROFILES_CONDITION_VARIABLE_CHANGED:
            taskrunner_queue(column_data[3]);
            if (condition_type != PROFILES_CONDITION_VARIABLE_CHANGED) {
                storage_exec_noresult(&g_engine.storage, "INSERT OR REPLACE INTO " _PROFILES_INTERNAL_TABLE " (profile_id, state, last_run) VALUES (%s, %i, %ld)", column_data[0], state, g_engine.now);
            }
            break;

        case PROFILES_CONDITION_CUSTOM:
            if (taskrunner_eval(column_data[6])) {
                if (state == PROFILES_STATE_INACTIVE) { // @todo @fixme atoi segfaults if column isn't a valid number
                    // Was inactive
                    taskrunner_queue(column_data[2]); // Run enter task
                    state = PROFILES_STATE_ACTIVE;

                    storage_exec_noresult(&g_engine.storage, "INSERT OR REPLACE INTO " _PROFILES_INTERNAL_TABLE " (profile_id, state, last_run) VALUES (%s, %i, %ld)", column_data[0], state, g_engine.now);
                }

                taskrunner_queue(column_data[3]); // Run tick task

                storage_exec_noresult(&g_engine.storage, "INSERT OR REPLACE INTO " _PROFILES_INTERNAL_TABLE " (profile_id, state, last_run) VALUES (%s, %i, %ld)", column_data[0], state, g_engine.now);
            } else {
                if (state == PROFILES_STATE_ACTIVE) {
                    // Was active
                    taskrunner_queue(column_data[4]); // Run exit task
                    state = PROFILES_STATE_INACTIVE;

                    storage_exec_noresult(&g_engine.storage, "INSERT OR REPLACE INTO " _PROFILES_INTERNAL_TABLE " (profile_id, state, last_run) VALUES (%s, %i, %i)", column_data[0], state, g_engine.now);
                }
            }
            break;
    }

    return 0;
}

/**
 * Find matching profiles to queue based on condition type and a custom parameter.
 */
int profiles_queue(profiles_t *profiles, int condition_type, const char *data)
{
    (void)profiles;

    switch (condition_type) {
        case PROFILES_CONDITION_VARIABLE_CHANGED:
            // Find all with condition_custom == data
            storage_exec_result_(&g_engine.storage, (void*)(intptr_t)condition_type, _profiles_run_callback,
                "SELECT id, name, enter_task, tick_task, exit_task, condition_type, condition_custom, state, last_run, tick_rate"
                " FROM " _PROFILES_TABLE " LEFT JOIN " _PROFILES_INTERNAL_TABLE " ON profile_id = id"
                " WHERE condition_type = %i AND condition_custom = \"%s\"", condition_type, data);
            break;

        case PROFILES_CONDITION_CUSTOM:
        case PROFILES_CONDITION_ALWAYS:
            // Run all tasks with this columns
            // @todo WHERE last_run + period < now
            storage_exec_result_(&g_engine.storage, (void*)(intptr_t)condition_type, _profiles_run_callback,
                "SELECT id, name, enter_task, tick_task, exit_task, condition_type, condition_custom, state, last_run, tick_rate"
                " FROM " _PROFILES_TABLE " LEFT JOIN " _PROFILES_INTERNAL_TABLE " ON profile_id = id"
                " WHERE condition_type = %i", condition_type, data);
            break;
    }

    return 0;
}
