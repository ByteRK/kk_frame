#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-05-22 15:42:58
 # @LastEditTime: 2025-05-28 11:48:16
 # @FilePath: /kk_frame/script/updating.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

USB_MOUNT_PREFIX="/vendor/udisk"         # U盘挂载目录前缀
STOP_APP=""             # 关闭的应用程序
STOP_APP_PARAM=""       # 关闭的应用程序参数
START_APP=""            # 启动的应用程序
START_APP_PARAM=""      # 启动的应用程序参数

# 检测所有可能的U盘分区
detect_usb_partitions() {
    partitions=""
    for dev in /sys/block/sd*; do
        if [ -e "$dev/device" ]; then
            for part in "$dev"/sd*[0-9]; do
                part_name=$(basename "$part")
                mount_point="${USB_MOUNT_PREFIX}_${part_name}"
                partitions="$partitions $mount_point"
            done
        fi
    done
    echo "$partitions"
}

if [ -n "$STOP_APP" ]; then
    killall "$STOP_APP" # 停止应用程序
fi
if [ -n "$START_APP" ]; then
    $START_APP $START_APP_PARAM & # 启动应用程序
fi


while true; do
    # 检测U盘是否存在
    usb_partitions=$(detect_usb_partitions)

    if [ -n "$usb_partitions" ]; then
        echo "USB disk detected: $usb_partitions"
        # 可以在这里添加你对U盘的处理逻辑，比如进行更新
        # 处理完后可以退出循环，或继续检测
        sleep 2  # 等待2秒再检测
    else
        echo "USB disk has been removed."
        break
    fi
done
echo "USB disk has been removed"


if [ -n "$START_APP" ]; then
    killall "$START_APP" # 停止应用程序
fi
if [ -n "$STOP_APP" ]; then
    $STOP_APP $STOP_APP_PARAM & # 启动应用程序
fi