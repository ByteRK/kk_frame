#!/bin/sh
#set -x
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-11-23 02:05:28
 # @LastEditTime: 2026-02-09 18:37:31
 # @FilePath: /kk_frame/board/run.sh
 # @Description: Startup Script
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

# Set the application name and arguments
APP=kk_frame
ARGS=""

# Go to the script directory
cd $(dirname $0)

# Run Some Commands Before Start
chmod +x $APP
chmod +x ./*.sh
. ./env.sh
. ./before.sh

(
count=0

# Start Process
./$APP $ARGS --resetCount=$count >/dev/null 2>/dev/null &
sleep 5

# If the process does not exist, replace the program file with the backup
if ! pgrep -f "./$APP" > /dev/null; then
    mount -o remount,rw /customer/
    cp -f $APP.bak $APP
    chmod +x $APP
    sync
fi

# Enter the process guard
while true; do
   if [ ! -d "/tmp/customer" ] && ! pgrep -f "./$APP" > /dev/null; then
       current_time=$(date "+%Y-%m-%d %H:%M:%S")
       count=$(expr $count + 1);
       echo -e "\033[33m $current_time.$milliseconds : APP RESTART \033[0m"
       ./$APP --resetCount=$count >/dev/null 2>/dev/null &
   fi
   sleep 2
done
)&
