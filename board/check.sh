#!/bin/sh
#set -x
cd $(dirname $0)

APP=kk_frame

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo 0
else
    echo 1
fi
