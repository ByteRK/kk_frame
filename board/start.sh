#!/bin/sh
#set -x
cd $(dirname $0)

APP=kk_frame
ARGS="-R 0"

. ./env.sh

[ -f "$APP.new" ] && mv $APP.new $APP
chmod +x $APP

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo "Starting $APP..."
    ./$APP $ARGS &
else
    echo "$APP is already running with PID: $pid"
fi

# sleep 1
# pid=$(pidof $APP)
# if [ -z "$pid" ]; then
#     [ -d bak ] && cp -rf bak/* /data/
# fi
