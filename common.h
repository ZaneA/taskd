#ifndef __COMMON_H
#define __COMMON_H

#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "storage.h"
#include "taskrunner.h"
#include "variables.h"
#include "profiles.h"
#include "plugins.h"

typedef struct {
    int debug;
    int tick_rate;
    long now;
    storage_t storage;
    taskrunner_t taskrunner;
    variables_t variables;
    profiles_t profiles;
    plugins_t plugins;
} engine_t;

engine_t g_engine;

#endif
