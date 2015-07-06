#include "common.h"
#include "storage.h"
#include "taskrunner.h"
#include "variables.h"
#include "profiles.h"
#include "plugins.h"

#include <time.h>

// Linux / Signal handling
#include <signal.h>
#include <unistd.h>

static volatile int running = 1;

#ifdef DEBUG
static volatile int debug = 1;
#else
static volatile int debug = 0;
#endif

void signal_handler(int signal)
{
    switch (signal) {
        case SIGINT:
            running = 0;
            break;

        case SIGHUP:
            debug = !debug;
            break;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    signal(SIGINT, signal_handler); // Handle ctrl-c for nice shutdown
    signal(SIGHUP, signal_handler); // Handle SIGHUP for debug printout

    // Config
    const char *database = getenv("TASKD_DATABASE");
    const char *plugins = getenv("TASKD_PLUGINS");

    // Abort if no values are set (@todo or use defaults?)
    if (database == NULL || plugins == NULL) {
        fprintf(stderr, "TASKD_DATABASE, TASKD_PLUGINS, and TASKD_TICK_RATE must be defined in environment.\n");
        exit(1);
    }

    // Set the default tick rate
    // @todo set minimum and maximum tick rate to override plugins/tasks?
    // This is in nanoseconds
    int tick_rate = atoi(getenv("TASKD_TICK_RATE"));

    fprintf(stderr, "Initializing subsystems.\n");

    g_engine.debug = debug;

    // Init subsystems
    storage_init(&g_engine.storage, database);
    atexit(storage_shutdown);

    taskrunner_init(&g_engine.taskrunner);
    atexit(taskrunner_shutdown);

    variables_init(&g_engine.variables);
    atexit(variables_shutdown);

    profiles_init(&g_engine.profiles);
    atexit(profiles_shutdown);

    plugins_init(&g_engine.plugins, plugins);
    atexit(plugins_shutdown);

    // Mainloop
    while (running) {
        g_engine.debug = debug;
        g_engine.tick_rate = tick_rate; // Reset tick rate each loop

        // Add high resolution time (millisecond resolution)
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
        g_engine.now = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);

        // Call a tick function on plugins so they can set new variables if needed
        plugins_tick(&g_engine.plugins);

        // Queue profiles to be run
        profiles_queue(&g_engine.profiles, PROFILES_CONDITION_ALWAYS, NULL); // Run all "always" profiles
        profiles_queue(&g_engine.profiles, PROFILES_CONDITION_CUSTOM, NULL); // Maybe run "custom" profiles

        // If we're in DEBUG mode we print out the SQLite tables each tick
        if (g_engine.debug) {
            storage_debug(&g_engine.storage);
        }

        // Run all the queued tasks here
        taskrunner_run_queued();

        // This should sleep exactly as long as requested for the next plugin to tick.
        usleep(g_engine.tick_rate * 1000);
    }

    fprintf(stderr, "Caught SIGINT, cleaning up.\n");

    return 0;
}
