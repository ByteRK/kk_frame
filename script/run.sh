#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-11-23 02:05:28
 # @LastEditTime: 2025-01-15 23:45:36
 # @FilePath: /kk_frame/script/run.sh
 # @Description: Startup Script
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

NAME=kk_frame
RUN_DIR=/customer/app

cd $RUN_DIR
. ./env.sh

(
count=0

# Start Process
./$NAME --resetCount=$count  >/dev/null 2>/dev/null &
sleep 5

# If the process does not exist, replace the program file with the backup
if ! pgrep -f "./$NAME" > /dev/null; then
    mount -o remount,rw /customer/
    cp -f $NAME.bak $NAME
    chmod +x $NAME
    sync
fi

# Enter the process guard
while true; do
   if [ ! -d "/tmp/customer" ] && ! pgrep -f "./$NAME" > /dev/null; then
       current_time=$(date "+%Y-%m-%d %H:%M:%S")
       count=$(expr $count + 1);
       echo -e "\033[33m $current_time.$milliseconds : APP RESTART \033[0m"
       ./$NAME --resetCount=$count  >/dev/null 2>/dev/null  &
   fi
   sleep 2
done
)&
