#!/bin/sh
#set -x

cd $(dirname $0)

echo "Restart app"

./stop.sh
sleep 1
./start.sh
