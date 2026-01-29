#!/bin/sh
#set -x

###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-05-22 15:42:58
 # @LastEditTime: 2026-01-29 17:27:18
 # @FilePath: /kk_frame/script/prebuild.sh
 # @Description: 编译前准备脚本
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

cd $(dirname $0) # 切换到当前脚本所在目录

echo -e "\033[32m[Prebuild] Generating version number...\033[0m"
../date2ver

echo -e "\033[32m[Prebuild] Generate font configuration...\033[0m"
./fonts.sh

echo -e "\033[32m[Prebuild] Check for changes in the number of files...\033[0m"
./file_count.sh