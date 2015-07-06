#include "common.h"
#include "plugins.h"

// private


// public

/**
 * Initialize plugin store.
 */
int plugins_init(plugins_t *plugins, const char *_plugins)
{
    assert(plugins != NULL);

    // Plugins table
    storage_exec_noresult(
        &g_engine.storage,
        "CREATE TABLE IF NOT EXISTS " _PLUGINS_TABLE
        "("
            "id INTEGER PRIMARY KEY,"
            "idx INTEGER,"
            "path VARCHAR(48),"
            "tick_rate INTEGER,"
            "last_run INTEGER"
        ");"
    );

    plugin_api.get = variables_get;
    plugin_api.set = variables_set;

    // Load plugins
    char *plugin = NULL;

    plugin = strtok((char*)_plugins, " ");

    while (plugin != NULL) {
        printf("Loading plugin %s...\n", plugin);
        plugins_load(&g_engine.plugins, plugin);
        plugin = strtok(NULL, " ");
    }

    return 0;
}

/**
 * Shutdown plugin store.
 */
void plugins_shutdown()
{
    for (int i = 0; i < MAX_PLUGINS; i++) {
        if (g_engine.plugins.plugins[i].handle != NULL) {
            if (g_engine.debug) {
                fprintf(stderr, "plugins_shutdown: Unloading plugin %p\n", g_engine.plugins.plugins[i].handle);
            }
            g_engine.plugins.plugins[i].shutdown(&plugin_api);
            dlclose(g_engine.plugins.plugins[i].handle);
        }
    }
}

int plugins_load(plugins_t *plugins, const char *path)
{
    static int idx = 0;

    plugins->plugins[idx].handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);

    if (plugins->plugins[idx].handle == NULL) {
        fprintf(stderr, "plugins_load: %s\n", dlerror());
        return 1;
    }

    *(void**)&(plugins->plugins[idx].init) = dlsym(plugins->plugins[idx].handle, "plugin_init");
    *(void**)&(plugins->plugins[idx].shutdown) = dlsym(plugins->plugins[idx].handle, "plugin_shutdown");
    *(void**)&(plugins->plugins[idx].tick) = dlsym(plugins->plugins[idx].handle, "plugin_tick");

    int tick_rate = plugins->plugins[idx].init(&plugin_api);

    storage_exec_noresult(&g_engine.storage, "INSERT INTO " _PLUGINS_TABLE " (idx, path, tick_rate, last_run) VALUES (%i, \"%s\", %i, %i);", idx++, path, tick_rate, 0);

    return 0;
}

int plugins_tick(plugins_t *plugins)
{
    for (int i = 0; i < MAX_PLUGINS; i++) {
        // @todo if plugins->plugins[i].last_run + plugins->plugins[i].period < now
        if (plugins->plugins[i].tick != NULL) {
            if (g_engine.debug) {
                fprintf(stderr, "plugins_tick: Plugin %p tick\n", plugins->plugins[i].handle);
            }
            plugins->plugins[i].tick(&plugin_api);
        }
    }

    return 0;
}
