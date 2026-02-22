#!/bin/sh
#set -x
cd $(dirname $0)

APP_PATH="$(pwd)"

export LANG=zh_CN.UTF-8
export TZ=Asia/Shanghai

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$APP_PATH/lib
export FONTCONFIG_PATH=$APP_PATH/fonts
export SCREEN_MARGINS=0,0,0,0

export DISPLAY_ROTATE=0
#export DEV_MODE=0

ulimit -c 0
