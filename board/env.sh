#!/bin/sh
#set -x
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-06-05 02:05:58
 # @LastEditTime: 2026-02-09 18:37:21
 # @FilePath: /kk_frame/board/env.sh
 # @Description: 环境配置脚本
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

APP_PATH="$(pwd)"

export LANG=zh_CN.UTF-8
export TZ=Asia/Shanghai

export LD_LIBRARY_PATH=$APP_PATH/lib/$LD_LIBRARY_PATH
export FONTCONFIG_PATH=$APP_PATH/fonts
export SCREEN_MARGINS=0,0,0,0

export DISPLAY_ROTATE=0
export DEV_MODE=0

ulimit -c 0
