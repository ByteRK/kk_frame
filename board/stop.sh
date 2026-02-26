#!/bin/sh
#set -x

APP=kk_frame

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo "$APP is not running"
else
    kill -9 $pid
    echo "$APP has been stopped"
fi