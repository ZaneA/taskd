# taskd

A tasker-like daemon designed to execute things based on other things.

This is a pre-alpha hack and is full of `sprintf` and glorious potential buffer overruns.

## High-level overview

taskd uses SQLite for storage of profiles, tasks, variables, and several internal states (balanced between disk and memory tables as appropriate).

Lua is used for execution and currently loads all default libs. Lua is modified to GET and SET variables into an in-memory SQLite table.

A simple plugin API is provided to GET/SET these variables, and to allow adding additional script functions in the future.

Like Tasker, the taskd execution engine is based around defining Profiles and Tasks.

### Tasks

A task has a name, description, and a Lua script to be run. For example you might have a task called "Lights On" that contains the script:

```lua
os.execute("lights_on.py")
```

and a similar one for "Lights Off".

### Profiles

Profiles define a context for when tasks should be run. A profile has several modes (defined in `profiles.h`):

- PROFILES_CONDITION_ALWAYS
  In this mode, the `tick_task` will run every "tick".

- PROFILES_CONDITION_CUSTOM
  In this mode, the script defined in the `condition_custom` column will be run every "tick".
  If the condition is met, the profile becomes ACTIVE and the `enter_task` is run. The `tick_task` is then run for each tick after this.
  Finally, when the condition is no longer true, the `exit_task` is run.

- PROFILES_CONDITION_VARIABLE_CHANGED
  In this mode, the `tick_task` is run when a variable is updated that matches the `condition_custom` column.

An example profile to turn off the lights when watching a movie might look like:

```lua
return MEDIA_ISPLAYING == "1"
```

The `enter_task` for this would be `Lights Off`, and the `exit_task` for this would be `Lights On`.

## Plugins

### p_core

The core plugin will provide some very basic variables for scripts to use. Currently this is some device info and date/time info. See `p_core.c` for more.

### p_httpapi

The httpapi plugin provides a very basic web server using libmicrohttpd that outputs JSON along with a basic web UI (at http://127.0.0.1:8080/index.html). Currently the functionality is limited to:

`http://127.0.0.1:8080/variables?key=VAR` and `http://127.0.0.1:8080/variables?key=VAR&value=new%20value`

This will be extended to allow introspecting all data and manipulating profiles/tasks.

![taskd web](http://i.imgur.com/UwIZCB1.png)

### p_replicate

The replicate plugin listens to variable changes and if a `TASKD_REPLICATE_HOST` option is provided (e.g. `http://remote:8080`) all variable values will be replicated to this remote instance.

Useful to run a local taskd with special plugins and replicate these to a hub elsewhere for processing.

## Installing

Clone this repo, make sure you have the following installed:

- lua5.1 (5.2 will NOT work currently)
- sqlite3
- libmicrohttpd if you want to build the httpapi
- libjansson (tested with 2.7) if you want to build the httpapi
- libcurl if you want to build p_replicate

Simply run `make taskd`, and `make <plugin name>` (for each plugin listed above). If need be modify `run_taskd.sh` and run `chmod +x run_taskd.sh`.

Finally if all goes well you should be able to `./run_taskd.sh`.

## Usage

There is a basic web UI available at http://127.0.0.1:8080/index.html by default. This will eventually allow the creation of profiles/tasks/variables. For now the database must be manipulated by hand (i.e. by using the `sqlite3` CLI).
