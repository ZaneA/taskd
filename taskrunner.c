#include "common.h"
#include "taskrunner.h"

// private

static int l_variables_get(lua_State *L)
{
    const char *key = luaL_checkstring(L, 1);

    lua_pushstring(L, variables_get(key));

    return 1;
}

static int l_variables_set(lua_State *L)
{
    const char *key = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);

    variables_set(key, "%s", value);

    return 0;
}

static int l_sql(lua_State *L)
{
    const char *sql = luaL_checkstring(L, 1);

    lua_pushstring(L, storage_exec_result(&g_engine.storage, "%s", sql));

    return 1;
}

// public

/**
 * Initialize taskrunner.
 */
int taskrunner_init(taskrunner_t *taskrunner)
{
    assert(taskrunner != NULL);

    // Tasks and queued tasks table
    storage_exec_noresult(
        &g_engine.storage,
        "CREATE TABLE IF NOT EXISTS " _TASKS_TABLE
        "("
            "id INTEGER PRIMARY KEY,"
            "name VARCHAR(50),"
            "description TEXT,"
            "script TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS " _QUEUED_TASKS_TABLE
        "("
            "script TEXT"
        ");"
    );

    // Set up Lua interpreter
    taskrunner->lua = luaL_newstate();
    luaL_openlibs(taskrunner->lua);

    // Register some basic functions
    // Ideally any undefined function in lua gets thrown to a taskrunner_execute_function() callback
    // This can then be passed on to a loaded lib.
    taskrunner_register("variables_get", l_variables_get);
    taskrunner_register("variables_set", l_variables_set);
    taskrunner_register("sql", l_sql);

    // This overrides what happens when a variable is GET or SET, we redirect it to the
    // variables table in SQLite :)
    // @todo only uppercase variables should be put into SQLite, others should be local to script.
    // @todo should be a better way to do this in Lua 5.2, this only works in 5.1
    if (luaL_dostring(
            taskrunner->lua,
            "do\
                local function set(t,name,value)\
                    variables_set(name, value)\
                end\
\
                local function get(t,name)\
                    return variables_get(name)\
                end\
\
                setmetatable(getfenv(),{__index=get,__newindex=set})\
            end"
        )) {
        fprintf(stderr, "taskrunner_init: %s\n", lua_tostring(g_engine.taskrunner.lua, -1));
        lua_pop(g_engine.taskrunner.lua, 1);
        exit(1); // Fail if error in init
    }

    return 0;
}

/**
 * Shutdown taskrunner.
 */
void taskrunner_shutdown()
{
    assert(g_engine.taskrunner.lua != NULL);

    lua_close(g_engine.taskrunner.lua);
}

/**
 * Register a script function.
 */
void taskrunner_register(const char *key, lua_CFunction lua_func)
{
    lua_pushcfunction(g_engine.taskrunner.lua, lua_func);
    lua_setglobal(g_engine.taskrunner.lua, key);
}

/**
 * Queue a task to be run.
 */
void taskrunner_queue(const char *task)
{
    storage_exec_noresult(&g_engine.storage, "INSERT INTO " _QUEUED_TASKS_TABLE " (script) SELECT script FROM " _TASKS_TABLE " WHERE name = \"%s\" OR id = \"%s\" LIMIT 1", task, task);
}

/**
 * Eval a task from first column.
 */
int _taskrunner_run_callback(void *user, int columns, char **column_data, char **column_names)
{
    (void)user;
    (void)columns;
    (void)column_names;

    taskrunner_eval(column_data[0]);

    return 0;
}

/**
 * Run all the queued tasks.
 */
void taskrunner_run_queued()
{
    storage_exec_result_(&g_engine.storage, NULL, _taskrunner_run_callback, "SELECT script FROM " _QUEUED_TASKS_TABLE "; DELETE FROM " _QUEUED_TASKS_TABLE);
}

/**
 * Eval a task (some lua code).
 */
int taskrunner_eval(const char *script)
{
    if (g_engine.debug) {
        printf("taskrunner_eval: %s\n", script);
    }

    if (luaL_dostring(g_engine.taskrunner.lua, script)) {
        fprintf(stderr, "taskrunner_eval: %s\n", lua_tostring(g_engine.taskrunner.lua, -1));
        lua_pop(g_engine.taskrunner.lua, 1);
        return 0;
    }

    int ret = 0;

    // If there is a return value
    if (lua_gettop(g_engine.taskrunner.lua)) {
        ret = lua_toboolean(g_engine.taskrunner.lua, -1);
        lua_pop(g_engine.taskrunner.lua, 1);
    }

    return ret;
}
