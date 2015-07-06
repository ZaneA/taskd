#ifndef __TASKRUNNER_H
#define __TASKRUNNER_H

#define _TASKS_TABLE _STORAGE_PERSISTENT ".tasks"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef struct {
    lua_State *lua;
} taskrunner_t;

int taskrunner_init(taskrunner_t *taskrunner);
void taskrunner_shutdown();

void taskrunner_register(const char *key, lua_CFunction lua_func);
void taskrunner_run(const char *task);
int taskrunner_eval(const char *script);

#endif
