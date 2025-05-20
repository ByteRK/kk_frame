#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-20 10:57:56
 # @FilePath: /hana_frame/script/run.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 


NAME=hana_frame
BAKDIR=bakFiles
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
    echo "Process $NAME failed to start, starting recovery from backup..."
    
    # 挂载为可写
    mount -o remount,rw /customer/
    
    # 遍历备份目录并恢复文件
    find "$BAKDIR" -type f | while read -r backup_file; do
        # 提取原始路径（去掉BAKDIR前缀）
        target_file="${backup_file#$BAKDIR/}"
        target_full_path="/$target_file"  # 假设原始路径在/下
        
        # 创建目标目录
        mkdir -p "$(dirname "$target_full_path")"
        
        # 复制文件（保留属性）
        if cp -af "$backup_file" "$target_full_path"; then
            echo "has resumed: $backup_file -> $target_full_path"
            # 如果是主程序，添加可执行权限
            if [ "$(basename "$target_file")" = "$NAME" ]; then
                chmod +x "$target_full_path"
            fi
        else
            echo "recovery failed: $backup_file"
        fi
    done
    
    sync
    echo "Recovery complete, try booting again $NAME..."
    ./$NAME --resetCount=$count >/dev/null 2>&1 &
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
