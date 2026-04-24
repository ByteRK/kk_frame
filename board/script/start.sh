#!/bin/sh
#set -x

cd $(dirname $0)

APP=$(sh ./name.sh)
ARGS="-R 0"

source ./env.sh
cd ../

[ -f "$APP.new" ] && mv $APP.new $APP
[ -f "$APP.pak.new" ] && mv $APP.pak.new $APP.pak

pid=$(pidof $APP)
if [ -z "$pid" ]; then
    echo "Starting $APP..."
    chmod +x $APP
    ./$APP $ARGS &
else
    echo "$APP is already running with PID: $pid"
fi

sleep 1
pid=$(pidof $APP)
if [ -z "$pid" ]; then
    [ -d bak ] && cp -rf bak/* ./
fi
