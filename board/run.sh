#!/bin/sh
#set -x
cd $(dirname $0)

chmod +x ./*.sh
[ -f "before.sh" ] && sh before.sh

while [ 1 = 1 ]; do
    check=$(sh check.sh)
    if [ "$check" -eq 0 ]; then
        sh start.sh
    fi
    sleep 1
    if [ -f STOP ]; then
        break
    fi
done
