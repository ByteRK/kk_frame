#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2025-05-03 21:10:04
 # @LastEditTime: 2025-05-03 21:10:29
 # @FilePath: /kk_frame/script/mount.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2025 by Ricken, All Rights Reserved. 
 # 
### 

ifconfig wlan0 down
ifconfig eth0 up
ifconfig eth0 10.0.0.224
mount -t nfs -o nolock 10.0.0.88:/home/nfs/000000  /mnt
mount -o remount,rw /customer