#!/bin/bash
#set -x

cd $(dirname $0)

APP=$(sh ./name.sh)

pid=$(pidof $APP)
touch STOP
if [ -z "$pid" ]; then
    echo "$APP is not running"
else
    kill -9 $pid
    echo "$APP has been stopped"
fi

# kill -9 无法让应用自己写 magic close，这里补一刀，
# 避免停应用后系统被硬件看门狗复位（设备不存在/未武装时脚本会自行跳过）
if [ -f "./watchdog.sh" ]; then
    sh ./watchdog.sh off >/dev/null 2>&1
fi