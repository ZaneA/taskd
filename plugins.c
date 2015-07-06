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
            "path VARCHAR(48)"
        ");"
    );

    // Connect up function pointers to plugin_api struct for plugins to use
    plugin_api.get = variables_get;
    plugin_api.set = variables_set;

    // Load plugins, split by " "
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

            // Call shutdown function
            if (g_engine.plugins.plugins[i].shutdown != NULL) {
                g_engine.plugins.plugins[i].shutdown(&plugin_api);
            }

            // Close shared object
            dlclose(g_engine.plugins.plugins[i].handle);
        }
    }
}

/**
 * Load a plugin and connect function pointers.
 */
int plugins_load(plugins_t *plugins, const char *path)
{
    static int idx = 0;

    // Open shared object
    plugins->plugins[idx].handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);

    if (plugins->plugins[idx].handle == NULL) {
        fprintf(stderr, "plugins_load: %s\n", dlerror());
        return 1;
    }

    // Look up symbols, the casts here look disgusting but -pedantic complains otherwise
    *(void**)&(plugins->plugins[idx].init) = dlsym(plugins->plugins[idx].handle, "plugin_init");
    *(void**)&(plugins->plugins[idx].shutdown) = dlsym(plugins->plugins[idx].handle, "plugin_shutdown");
    *(void**)&(plugins->plugins[idx].tick) = dlsym(plugins->plugins[idx].handle, "plugin_tick");

    // If init was found
    if (plugins->plugins[idx].init != NULL) {
        plugins->plugins[idx].tick_rate = plugins->plugins[idx].init(&plugin_api);
    }

    // This is actually pointless right now, but should store a plugin description for discoverability
    storage_exec_noresult(&g_engine.storage, "INSERT INTO " _PLUGINS_TABLE " (idx, path) VALUES (%i, \"%s\");", idx++, path);

    return 0;
}

int plugins_tick(plugins_t *plugins)
{
    for (int i = 0; i < MAX_PLUGINS; i++) {
        // If a tick function has been looked up (ie. this is a non-null plugin), and we're OK to tick
        if (plugins->plugins[i].tick != NULL && (plugins->plugins[i].last_run + plugins->plugins[i].tick_rate < g_engine.now)) {
            if (g_engine.debug) {
                fprintf(stderr, "plugins_tick: Plugin %p tick\n", plugins->plugins[i].handle);
            }

            // Run the tick function and update the returned tick_rate and last_run
            plugins->plugins[i].tick_rate = plugins->plugins[i].tick(&plugin_api);
            plugins->plugins[i].last_run = g_engine.now;

            // If returned tick rate is lower than current engine tick rate then replace it
            if (plugins->plugins[i].tick_rate > 0 && plugins->plugins[i].tick_rate < g_engine.tick_rate) {
                g_engine.tick_rate = plugins->plugins[i].tick_rate;
            }
        }
    }

    return 0;
}
