#!/bin/sh
###
 # @Author: cy
 # @Email: 964028708@qq.com
 # @Date: 2024-06-05 02:05:58
 # @LastEditTime: 2025-03-11 14:06:56
 # @FilePath: /cy_frame/script/env.sh
 # @Description: 环境配置脚本
 # @BugList: 
 # 
 # Copyright (c) 2024 by Cy, All Rights Reserved. 
 # 
### 

export PATH=$PATH:/config/wifi/
export LD_LIBRARY_PATH=/customer/lib:/config/wifi:$LD_LIBRARY_PATH
export FONTCONFIG_PATH=/customer/fonts
export LANG=zh_CN.UTF-8
export SCREEN_MARGINS=0,0,0,0
ulimit -c 0
