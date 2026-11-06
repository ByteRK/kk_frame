#!/bin/bash
#set -x

cd $(dirname $0)

# OTA
if [ -f "../ota/ota.sh" ]; then
    sh ../ota/ota.sh || echo "[before.sh] ota.sh failed: $?" >&2
fi

# 限制core文件大小为0
ulimit -c 0
