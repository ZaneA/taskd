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
            "id INTEGER PRIMARY KEY",
            "name VARCHAR(50),"
            "enter_task INTEGER,"
            "tick_task INTEGER,"
            "exit_task INTEGER,"
            "state INTEGER,"
            "condition_type INTEGER,"
            "condition_custom TEXT,"
            "tick_rate INTEGER,"
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

int _profiles_run_callback(void *user, int columns, char **column_data, char **column_names)
{
    (void)columns;
    (void)column_names;

    int condition_type = (intptr_t)user;

    //"SELECT name, enter_task, tick_task, exit_task, state, condition_type, condition_custom"
    switch (condition_type) {
        case PROFILES_CONDITION_ALWAYS:
        case PROFILES_CONDITION_VARIABLE_CHANGED:
            taskrunner_run(column_data[2]);
            break;

        case PROFILES_CONDITION_CUSTOM:
            if (taskrunner_eval(column_data[6])) {
                if (atoi(column_data[4]) == PROFILES_STATE_INACTIVE) { // @todo @fixme atoi segfaults if column isn't a valid number
                    // Was inactive
                    taskrunner_run(column_data[1]); // Run enter task
                    storage_exec_noresult(&g_engine.storage, "UPDATE " _PROFILES_TABLE " SET state = %i WHERE name = \"%s\"", PROFILES_STATE_ACTIVE, column_data[0]);
                }

                taskrunner_run(column_data[2]); // Run tick task
            } else {
                if (atoi(column_data[4]) == PROFILES_STATE_ACTIVE) {
                    // Was active
                    taskrunner_run(column_data[3]); // Run exit task
                    storage_exec_noresult(&g_engine.storage, "UPDATE " _PROFILES_TABLE " SET state = %i WHERE name = \"%s\"", PROFILES_STATE_INACTIVE, column_data[0]);
                }
            }
            break;
    }

    return 0;
}

int profiles_run(profiles_t *profiles, int condition_type, const char *data)
{
    (void)profiles;

    switch (condition_type) {
        case PROFILES_CONDITION_VARIABLE_CHANGED:
            // Find all with condition_custom == data
            storage_exec_result_(&g_engine.storage, (void*)(intptr_t)condition_type, _profiles_run_callback,
                "SELECT name, enter_task, tick_task, exit_task, state, condition_type, condition_custom"
                " FROM " _PROFILES_TABLE " WHERE condition_type = %i AND condition_custom = \"%s\"", condition_type, data);
            break;

        case PROFILES_CONDITION_CUSTOM:
        case PROFILES_CONDITION_ALWAYS:
            // Run all tasks with this columns
            // @todo WHERE last_run + period < now
            storage_exec_result_(&g_engine.storage, (void*)(intptr_t)condition_type, _profiles_run_callback,
                "SELECT name, enter_task, tick_task, exit_task, state, condition_type, condition_custom"
                " FROM " _PROFILES_TABLE " WHERE condition_type = %i", condition_type);
            break;
    }

    return 0;
}
