#!/bin/bash
#set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
APP_PATH=$(cd "$SCRIPT_DIR/.." && pwd)

export LANG="zh_CN.UTF-8"
export TZ="Asia/Shanghai"

export LD_LIBRARY_PATH="${APP_PATH}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export FONTCONFIG_PATH="${APP_PATH}/fonts"

export SCREEN_MARGINS="0,0,0,0"
export DISPLAY_ROTATE="270"
export VIDEO_ROTATION="270"

# export APP_AUTH_URL="http://172.16.230.66:1415/api/device/authorize"
# export DEV_MODE="0"
