#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-20 14:47:07
 # @FilePath: /hana_frame/script/run.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 


NAME=hana_frame
BAKDIR=bakFiles
RUN_DIR=/customer/app
MIN_UPTIME=300  # 要求程序至少运行5分钟才视为稳定
CHECK_INTERVAL=2 # 每隔2秒检查一次
UPGRADE_SCRIPT="upgrade.sh"  # 需要等待的脚本名

cd $RUN_DIR
. ./env.sh

(
count=0

# 启动进程函数
start_process() {
    ./$NAME --resetCount=$count >/dev/null 2>&1 &
    echo "Process $NAME started (attempt $((count+1)))"
    count=$((count+1))
}

# 恢复备份函数
recover_backup() {
    echo "Starting recovery from backup..."
    mount -o remount,rw /customer/ || echo "Mount RW failed"
    
    find "$BAKDIR" -type f | while read -r backup_file; do
        target_file="${backup_file#$BAKDIR/}"
        target_full_path="/customer/$target_file"
        
        mkdir -p "$(dirname "$target_full_path")"
        
        if cp -af "$backup_file" "$target_full_path"; then
            echo "Restored: $backup_file -> $target_full_path"
            [ "$(basename "$target_file")" = "$NAME" ] && chmod +x "$target_full_path"
        else
            echo "Failed to restore: $backup_file"
        fi
    done
    sync
}

# 等待其他升级脚本完成
wait_for_upgrade() {
    while pgrep -f "$UPGRADE_SCRIPT" > /dev/null; do
        echo "Waiting for $UPGRADE_SCRIPT to finish..."
        sleep 5
    done
}



# Start Process
start_process

wait_for_upgrade  # 先等待其他升级脚本完成

# If the process does not exist, replace the program file with the backup
if ! pgrep -f "./$NAME" > /dev/null; then
    recover_backup
    start_process
    sleep 5
fi

# Enter the process guard
while true; do
   if [ ! -d "/tmp/customer" ] && ! pgrep -f "./$NAME" > /dev/null; then
       top
   else 
       pid=$(pgrep -f "./$NAME")
       start_time=$(stat -c %Y /proc/$pid/)
       current_time=$(date +%s)
       uptime=$((current_time - start_time))
       
       if [[ $uptime -ge $MIN_UPTIME && -d "$BAKDIR" ]]; then
            mount -o remount,rw /customer
           echo "Process $NAME has run stably for $uptime seconds. Safe to remove backup."
           rm -rf "$BAKDIR"
           sync
           break  # 退出监控循环
       fi 
   fi
   sleep $CHECK_INTERVAL
done
)&
