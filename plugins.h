#ifndef __PLUGINS_H
#define __PLUGINS_H

#define _PLUGINS_TABLE _STORAGE_MEMORY ".plugins"
#define MAX_PLUGINS 32

#include <dlfcn.h>
#include "plugin_api.h"

typedef struct {
    plugin_t plugins[MAX_PLUGINS];
} plugins_t;

plugin_api_t plugin_api;

int plugins_init(plugins_t *plugins, const char *_plugins);
void plugins_shutdown();

int plugins_load(plugins_t *plugins, const char *path);
int plugins_tick(plugins_t *plugins);
void plugins_event(plugins_t *plugins, int event, void *event_data);

#endif
