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
    const char *database = getenv("TASKERD_DATABASE");
    const char *plugins = getenv("TASKERD_PLUGINS");

    // Abort if no values are set (@todo or use defaults?)
    if (database == NULL || plugins == NULL) {
        fprintf(stderr, "TASKERD_DATABASE, TASKERD_PLUGINS, and TASKERD_TICK_RATE must be defined in environment.\n");
        exit(1);
    }

    // Set the default tick rate
    // @todo set minimum and maximum tick rate to override plugins/tasks?
    // This is in nanoseconds
    int tick_rate = atoi(getenv("TASKERD_TICK_RATE")) * 1000;

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
        g_engine.next_tick = tick_rate; // Reset tick rate each loop

        // @todo add high resolution timer to g_engine struct

        // Call a tick function on plugins so they can set new variables if needed
        plugins_tick(&g_engine.plugins);

        // If we're in DEBUG mode we print out the SQLite tables each tick
        if (g_engine.debug) {
            storage_debug(&g_engine.storage);
        }

        // @todo make this function "queue" tasks to be run
        // This way we queue tasks on variable change instead of running them at that
        // exact point in time and potentially screwing up program flow or getting into a loop.
        // Might also be possible to run this stuff in a thread later this way.
        profiles_run(&g_engine.profiles, PROFILES_CONDITION_ALWAYS, NULL); // Run all "always" profiles
        profiles_run(&g_engine.profiles, PROFILES_CONDITION_CUSTOM, NULL); // Maybe run "custom" profiles

        // @todo run the actual tasks here
        //tasks_run(&g_engine.taskrunner, tasklist);

        // Pause for a second, this should be replaced by something smarter eventually
        // Probably a pause for the shortest time (500ms or so) and check which plugins need to tick
        // OR figure out when next tick needs to occur and sleep exactly that long
        // @todo add g_engine.next_tick, during plugins_tick() / profiles_run() if period is shorter than current next_time then update
        usleep(g_engine.next_tick);
    }

    fprintf(stderr, "Caught SIGINT, cleaning up.\n");

    return 0;
}
