#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-08 18:51:39
 # @FilePath: /hana_frame/script/run.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 


NAME=hana_frame
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
