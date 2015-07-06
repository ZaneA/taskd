#!/bin/bash
shopt -s nullglob
plugins=($(pwd)/plugins/*.so)

export TASKD_DATABASE=taskerd.db
export TASKD_PLUGINS="${plugins[@]}"
export TASKD_TICK_RATE=1000

./taskd
