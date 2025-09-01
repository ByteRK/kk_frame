#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-09-01 16:39:42
 # @FilePath: /hana_frame/script/env.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 

export PATH=$PATH:/config/wifi/
export LD_LIBRARY_PATH=/customer/lib:/config/wifi:$LD_LIBRARY_PATH
export FONTCONFIG_PATH=/customer/fonts
export LANG=zh_CN.UTF-8
export SCREEN_MARGINS=0,0,0,0

export DEV_MODE=0

ulimit -c 0
