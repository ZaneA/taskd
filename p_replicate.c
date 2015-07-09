#include "plugin_api.h"

#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>

char *replicate_host_env = NULL;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    (void)buffer;
    (void)userp;
    
    return size * nmemb;
}

/**
 * Initialize plugin.
 */
int plugin_init(plugin_api_t *plugin_api)
{
    (void)plugin_api;

    // Grab the HOST to replicate to
    replicate_host_env = getenv("TASKD_REPLICATE_HOST");

    return 0;
}

/**
 * Shutdown plugin.
 */
void plugin_shutdown(plugin_api_t *plugin_api)
{
    (void)plugin_api;
}

/**
 * Listen for variable changes.
 */
void plugin_event(plugin_api_t *plugin_api, int event, void *event_data)
{
    // If no host then do nothing
    if (replicate_host_env == NULL) {
        return;
    }

    switch (event) {
        case PLUGIN_EVENT_VARIABLE_CHANGED:
            {
                const char *key = event_data;
                const char *value = plugin_api->get(key);

                CURL *curl = curl_easy_init();

                if (curl != NULL) {
                    char url[256] = {0};
                    sprintf(url, "%s/variables?key=%s&value=%s", replicate_host_env, key, value);

                    curl_easy_setopt(curl, CURLOPT_URL, url);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                    curl_easy_perform(curl);
                    curl_easy_cleanup(curl);
                }
            }
            break;
    }
}
