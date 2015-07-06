#include "plugin_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#define PORT 8080

static struct MHD_Daemon *mhd_daemon = NULL;

int mhd_answer_to_connection(
    void *cls, struct MHD_Connection *connection,
    const char *url,
    const char *method, const char *version,
    const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    (void)method;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    plugin_api_t *plugin_api = (plugin_api_t*)cls;

    char buffer[256] = {0};

    if (!strcmp(url, "/variables")) {
        const char *key = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "key");
        const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");

        if (key != NULL && value != NULL) {
            plugin_api->set(key, value);
            sprintf(buffer, "%s", "OK");
        } else if (key != NULL) {
            sprintf(buffer, "%s", plugin_api->get(key));
        } else {
            // Print all
            // @todo
        }
    }

    if (!strcmp(url, "/tasks")) {
        const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
        const char *description = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "description");
        const char *script = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "script");

        if (name != NULL && description != NULL && script != NULL) {
            // Add task
        } else {
            // Print all
            // @todo
        }
    }

    if (!strcmp(url, "/profiles")) {
        const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
        const char *enter_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "enter_task");
        const char *tick_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "tick_task");
        const char *exit_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "exit_task");
        //const char *state = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "state");
        const char *condition_type = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "condition_type");
        const char *condition_custom = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "condition_custom");

        if (name != NULL && enter_task != NULL && tick_task != NULL
            && exit_task != NULL && condition_type != NULL && condition_custom != NULL) {
            // Add profile
        } else {
            // Print all
            // @todo
        }
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buffer), (void*)buffer, MHD_RESPMEM_MUST_COPY);

    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int plugin_init(plugin_api_t *plugin_api)
{
    // Spin up HTTP server
    mhd_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &mhd_answer_to_connection, (void*)plugin_api, MHD_OPTION_END);

    if (mhd_daemon == NULL) {
        return -1;
    }

    return 5000; // Tick rate (update once per second)
}

void plugin_shutdown(plugin_api_t *plugin_api)
{
    (void)plugin_api;

    if (mhd_daemon == NULL) {
        return;
    }

    // Shut down HTTP server
    MHD_stop_daemon(mhd_daemon);
    mhd_daemon = NULL;
}

int plugin_tick(plugin_api_t *plugin_api)
{
    (void)plugin_api;

    // Tick webserver

    // Handle various routes

    // /variables
    // /variables?key=BLAH&value=blah%20value
    // /variables?key=BLAH => blah value

    // plugin_api->set("BLAH", "%s", buf);
    // plugin_api->get("BLAH");

    // /tasks
    // /tasks/add?name=Task%20Name&description=Here%20is%20a%20description&script=--luacodehere

    // /profiles
    // /profiles/add?name=Task%20Name&description=Here%20is%20a%20description&enter_task=Task%20Name&tick_task=&exit_task=&condition_type=2

    return 5000;
}
