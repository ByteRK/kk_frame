#!/bin/bash
#set -x

cd $(dirname $0)

while [ 1 -eq 1 ]; do
    if [ -z "$(pidof wpa_supplicant)" ]; then
        echo "start service ok"
        wpa_supplicant -B -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf
    fi
    sleep 1
done
