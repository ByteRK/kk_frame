#!/bin/sh
#set -x

APP=$(sh ./name.sh)

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo "$APP is not running"
else
    kill -9 $pid
    echo "$APP has been stopped"
fi