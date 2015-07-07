#include "plugin_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <microhttpd.h>
#include <jansson.h>

// @todo grab from environment
#define PORT 8080

static struct MHD_Daemon *mhd_daemon = NULL;

int sql_callback(void *user, int columns, char **column_data, char **column_names)
{
    json_t *root = (json_t*)user;

    json_t *object = json_object();

    for (int i = 0; i < columns; i++) {
        json_object_set_new(object, column_names[i], json_string(column_data[i]));
    }

    json_array_append_new(root, object);

    return 0;
}

/**
 * Handle a client connection, best to check out the libmicrohttpd documentation for this one...
 */
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

    // All routes should return JSON
    json_t *root = NULL;

    if (!strcmp(url, "/variables")) {
        const char *key = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "key");
        const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");

        if (key != NULL && value != NULL) {
            plugin_api->set(key, value);
            root = json_pack("{s:s, s:s, s:s}", "key", key, "value", plugin_api->get(key), "status", "OK");
        } else if (key != NULL) {
            root = json_pack("{s:s, s:s, s:s}", "key", key, "value", plugin_api->get(key), "status", "OK");
        } else {
            json_t *arr = json_array();
            plugin_api->sql(arr, sql_callback, "SELECT * FROM memory.variables");
            root = json_pack("{s:o, s:s}", "variables", arr, "status", "OK");
        }
    } else if (!strcmp(url, "/tasks")) {
        const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
        const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
        const char *description = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "description");
        const char *script = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "script");

        if (id != NULL && name != NULL && description != NULL && script != NULL) {
            // Add or update task
            plugin_api->sql(NULL, NULL, "INSERT OR REPLACE INTO main.tasks "
                "(id, name, description, script) VALUES "
                "(%s, \"%s\", \"%s\", \"%s\")",
                id, name, description, script);
        }

        json_t *arr = json_array();
        plugin_api->sql(arr, sql_callback, "SELECT * FROM main.tasks");
        root = json_pack("{s:o, s:s}", "tasks", arr, "status", "OK");
    } else if (!strcmp(url, "/profiles")) {
        const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
        const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
        const char *enter_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "enter_task");
        const char *tick_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "tick_task");
        const char *exit_task = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "exit_task");
        const char *tick_rate = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "tick_rate");
        const char *condition_type = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "condition_type");
        const char *condition_custom = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "condition_custom");

        if (id != NULL && name != NULL && enter_task != NULL && tick_task != NULL
            && exit_task != NULL && tick_rate != NULL && condition_type != NULL && condition_custom != NULL) {
            // Add or update profile
            plugin_api->sql(NULL, NULL, "INSERT OR REPLACE INTO main.profiles "
                "(id, name, enter_task, tick_task, exit_task, tick_rate, condition_type, condition_custom) VALUES "
                "(%s, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")",
                id, name, enter_task, tick_task, exit_task, tick_rate, condition_type, condition_custom);
        }

        json_t *arr = json_array();
        plugin_api->sql(arr, sql_callback, "SELECT * FROM main.profiles");
        root = json_pack("{s:o, s:s}", "profiles", arr, "status", "OK");
    } else {
        char path[256] = {0};
        strcat(path, "web");
        strcat(path, url);

        struct stat stat_buf;
        stat(path, &stat_buf);

        int fd = open(path, O_RDONLY);

        if (fd > 0 && stat_buf.st_mode & S_IFREG) {
            struct MHD_Response *response = MHD_create_response_from_fd(stat_buf.st_size, fd);
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);

            return ret;
        } else {
            if (!(stat_buf.st_mode & S_IFREG)) {
                close(fd);
            }

            struct MHD_Response *response = MHD_create_response_from_buffer(strlen("404"), "404", MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header (response, "Content-Type", "text/html");
            int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
            MHD_destroy_response(response);

            return ret;
        }
    }

    // Dump JSON as string
    char *buffer = json_dumps(root, JSON_INDENT(2));
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buffer), (void*)buffer, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header (response, "Content-Type", "application/json");
    free(buffer);

    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

/**
 * Initialize plugin.
 */
int plugin_init(plugin_api_t *plugin_api)
{
    // Spin up HTTP server
    mhd_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &mhd_answer_to_connection, (void*)plugin_api, MHD_OPTION_END);

    if (mhd_daemon == NULL) {
        return -1;
    }

    return 5000; // Tick rate (update once per second)
}

/**
 * Shutdown plugin.
 */
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

/**
 * Plugin tick.
 */
int plugin_tick(plugin_api_t *plugin_api)
{
    (void)plugin_api;

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
