#include "plugin_api.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

/**
 * Initialize plugin.
 */
int plugin_init(plugin_api_t *plugin_api)
{
    (void)plugin_api;
    return 5000; // Tick rate (update once per second)
}

/**
 * Shutdown plugin.
 */
void plugin_shutdown(plugin_api_t *plugin_api)
{
    (void)plugin_api;
}

/**
 * Plugin tick.
 */
int plugin_tick(plugin_api_t *plugin_api)
{
    int event_base_return, error_base_return;
    Display *dpy = XOpenDisplay("");

    if (XScreenSaverQueryExtension(dpy, &event_base_return, &error_base_return)) {
        XScreenSaverInfo saver_info;
        XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), &saver_info);

        plugin_api->set("X11_IDLE", "%ld", saver_info.idle / 1000);
    }

    XCloseDisplay(dpy);

    // The return value how many milliseconds until tick should be called again.
    return 5000;
}
