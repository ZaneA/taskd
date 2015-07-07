#ifndef __PLUGIN_API_H
#define __PLUGIN_API_H

// Daemon function typedefs
typedef char* (*variables_get_func)(const char *key);
typedef void (*variables_set_func)(const char *key, const char *format, ...);
typedef void (*sql_func)(void *user, int(*callback)(void*, int, char**, char**), const char *format, ...);

// This struct is used by plugin to communicate with daemon
typedef struct {
    variables_get_func get;
    variables_set_func set;
    sql_func sql;
} plugin_api_t;

// Plugin function typedefs
typedef int (*plugin_init_func)(plugin_api_t *plugin_api);
typedef void (*plugin_shutdown_func)(plugin_api_t *plugin_api);
typedef int (*plugin_tick_func)(plugin_api_t *plugin_api);

// This struct is used by daemon to communicate with plugins
typedef struct {
    void* handle;
    int tick_rate;
    int last_run;
    plugin_init_func init;
    plugin_shutdown_func shutdown;
    plugin_tick_func tick;
} plugin_t;

#endif
