#!/bin/bash
shopt -s nullglob
plugins=($(pwd)/plugins/*.so)

# Core config
export TASKD_DATABASE=taskd.db
export TASKD_PLUGINS="${plugins[@]}"
export TASKD_TICK_RATE=1000
export TASKD_DEBUG=0

# Plugin config
export TASKD_HTTAPI_PORT=8080

./taskd
