#!/bin/sh
#set -x

cd $(dirname $0)

APP=$(sh ./name.sh)

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo 0
else
    echo 1
fi
