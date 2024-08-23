#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-06-05 02:05:28
 # @LastEditTime: 2024-08-22 16:45:04
 # @FilePath: /kk_frame/script/run.sh
 # @Description: 启动脚本
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

NAME=kk_frame

cd /customer/app
. ./env.sh

(
count=0
while true; do
   if [ ! -d "/tmp/customer" ] && ! pgrep -f "./$NAME" > /dev/null; then
       current_time=$(date "+%Y-%m-%d %H:%M:%S")
       count=$(expr $count + 1);
       #echo -e "\033[33m $current_time.$milliseconds : APP RESTART \033[0m"
       ./$NAME --resetCount=$count  >/dev/null 2>/dev/null  &
   fi
   sleep 3
done
)&