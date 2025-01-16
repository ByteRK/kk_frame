#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-06-05 02:05:58
 # @LastEditTime: 2025-01-15 23:04:09
 # @FilePath: /kk_frame/script/env.sh
 # @Description: 环境配置脚本
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

export PATH=$PATH:/config/wifi/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/customer/lib:/config/wifi/
export FONTCONFIG_PATH=/customer/fonts
export LANG=zh_CN.UTF-8
ulimit -c 0