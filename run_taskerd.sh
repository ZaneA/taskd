#!/bin/bash
shopt -s nullglob
plugins=($(pwd)/plugins/*.so)

export TASKERD_DATABASE=taskerd.db
export TASKERD_PLUGINS="${plugins[@]}"
export TASKERD_TICK_RATE=1000

./taskerd
