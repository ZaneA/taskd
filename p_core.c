#include "plugin_api.h"

#include <stdio.h>

#include <time.h>
#include <sys/utsname.h>

int plugin_init(plugin_api_t *plugin_api)
{
    // Only need to set these once so we do it during init
    // uname
    {
        struct utsname buf;
        uname(&buf);

        plugin_api->set("UNAME_SYSNAME", "%s", buf.sysname);
        plugin_api->set("UNAME_NODENAME", "%s", buf.nodename);
        plugin_api->set("UNAME_RELEASE", "%s", buf.release);
        plugin_api->set("UNAME_VERSION", "%s", buf.version);
        plugin_api->set("UNAME_MACHINE", "%s", buf.machine);
    }

    // Initialize timezone
    tzset();

    return 1000; // Tick rate (update once per second)
}

void plugin_shutdown(plugin_api_t *plugin_api)
{
    (void)plugin_api;
}

int plugin_tick(plugin_api_t *plugin_api)
{
    // Set up some date variables
    {
        time_t t = time(NULL);
        struct tm v;
        localtime_r(&t, &v);

        plugin_api->set("TIME", "%i", (int)t);
        plugin_api->set("DATE_CMP", "%02i%02i%02i", v.tm_hour, v.tm_min, v.tm_sec);
        plugin_api->set("DATE_SECONDS", "%02i", v.tm_sec);
        plugin_api->set("DATE_MINUTES", "%02i", v.tm_min);
        plugin_api->set("DATE_HOURS", "%i", v.tm_hour);
        plugin_api->set("DATE_DAY_OF_MONTH", "%i", v.tm_mday);
        plugin_api->set("DATE_MONTH", "%i", v.tm_mon);
        plugin_api->set("DATE_YEAR", "%i", v.tm_year);
        plugin_api->set("DATE_WEEK_DAY", "%i", v.tm_wday);
        plugin_api->set("DATE_DAY_OF_YEAR", "%i", v.tm_yday);
        plugin_api->set("DATE_IS_DST", "%i", v.tm_isdst);
    }

    return 1000;
}
