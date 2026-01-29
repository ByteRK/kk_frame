#!/bin/sh
#set -x

###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2026-01-29 17:13:58
 # @LastEditTime: 2026-01-29 17:44:45
 # @FilePath: /kk_frame/script/file_count.sh
 # @Description: 检查文件数量是否存在变化
 # @BugList: 
 # 
 # Copyright (c) 2026 by Ricken, All Rights Reserved. 
 # 
### 

cd $(dirname $0)/../ # 切换到项目根路径

# 设置要检查的目录列表（使用空格分隔）
DIRECTORIES="./src"

# 状态文件，记录上次的文件数量
STATE_FILE="./fileCountCache"

# 当检测到变化时要执行的命令
ALERT_COMMAND="touch ./CMakeLists.txt"

# 确保状态文件存在
touch "$STATE_FILE"

# 初始化变化标志
changes_found=0  # 使用数字代替布尔值

echo "开始检查文件数量..."
echo "检查时间: $(date)"

for directory in $DIRECTORIES; do
    # 检查目录是否存在
    if [ ! -d "$directory" ]; then
        echo "警告: 目录 $directory 不存在，跳过"
        continue
    fi
    
    # 获取当前文件数量
    current_count=$(find "$directory" -type f 2>/dev/null | wc -l)
    
    # 读取上次的文件数量
    last_count=$(grep "^${directory}:" "$STATE_FILE" 2>/dev/null | cut -d':' -f2)
    
    if [ -z "$last_count" ]; then
        # 第一次运行，记录初始状态
        echo "${directory}:${current_count}" >> "$STATE_FILE"
        echo "初始化: $directory - $current_count 个文件"
    elif [ "$current_count" -ne "$last_count" ]; then
        # 文件数量发生变化
        echo "检测到变化: $directory - 从 $last_count 变为 $current_count 个文件"
        changes_found=1
        
        # 更新状态文件
        if grep -q "^${directory}:" "$STATE_FILE"; then
            # 使用临时文件避免sed -i在不同系统上的兼容性问题
            temp_file=$(mktemp)
            sed "s|^${directory}:.*|${directory}:${current_count}|" "$STATE_FILE" > "$temp_file"
            mv "$temp_file" "$STATE_FILE"
        else
            echo "${directory}:${current_count}" >> "$STATE_FILE"
        fi
    else
        echo "正常: $directory - $current_count 个文件 (无变化)"
    fi
done

echo "检查完成"

if [ "$changes_found" -eq 1 ]; then
    eval "$ALERT_COMMAND"

    echo "温馨提示: 检测到文件数量变化，已重新触发Cmake构建，建议重新make当前项目"
    exit 0
else
    echo "温馨提示: 未检测到文件数量变化"
    exit 0
fi