#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-08 18:53:24
 # @FilePath: /hana_frame/script/updating.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 

USB_DIR="/vendor/udisk_sda1"  # U盘挂载路径
STOP_APP=""             # 关闭的应用程序
STOP_APP_PARAM=""       # 关闭的应用程序参数
START_APP=""            # 启动的应用程序
START_APP_PARAM=""      # 启动的应用程序参数


if [ -n "$STOP_APP" ]; then
    killall "$STOP_APP" # 停止应用程序
fi
if [ -n "$START_APP" ]; then
    $START_APP $START_APP_PARAM & # 启动应用程序
fi


while [ -d "$USB_DIR" ]; do
    #echo "目录存在: $USB_DIR"
    sleep 2  # 每秒检查一次U盘是否存在
done
echo "USB disk has been removed"


if [ -n "$START_APP" ]; then
    killall "$START_APP" # 停止应用程序
fi
if [ -n "$STOP_APP" ]; then
    $STOP_APP $STOP_APP_PARAM & # 启动应用程序
fi