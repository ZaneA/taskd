#ifndef __PLUGIN_API_H
#define __PLUGIN_API_H

typedef char* (*variables_get_func)(const char *key);
typedef void (*variables_set_func)(const char *key, const char *format, ...);

typedef struct {
    variables_get_func get;
    variables_set_func set;
} plugin_api_t;

typedef int (*plugin_init_func)(plugin_api_t *plugin_api);
typedef void (*plugin_shutdown_func)(plugin_api_t *plugin_api);
typedef int (*plugin_tick_func)(plugin_api_t *plugin_api);

typedef struct {
    void* handle;
    plugin_init_func init;
    plugin_shutdown_func shutdown;
    plugin_tick_func tick;
} plugin_t;

#endif
