###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-07-11 14:18:06
 # @LastEditTime: 2024-07-31 00:10:37
 # @FilePath: /kk_frame/script/pull.sh
 # @Description: 更新脚本 - 调试用
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

NAME=kk_frame
IPADDR=192.168.31.3
SERVEIP=192.168.31.2

killall $NAME
mount -o remount,rw /customer
ifconfig eth0 $IPADDR up
tftp -g -r $NAME $SERVEIP -b 4096

if [ "$1" = "-p" ]; then
    tftp -g -r $NAME.pak  $SERVEIP -b 4096
fi

if [ "$1" = "-a" ]; then
    tftp -g -r $NAME.pak  $SERVEIP -b 4096
    tftp -g -r cdroid.pak  $SERVEIP -b 4096
    cd /customer/lib/
    tftp -g -r libcdroid.so  $SERVEIP -b 4096
fi

chmod +x $NAME