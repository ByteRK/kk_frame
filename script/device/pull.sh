###
 # @Author: cy
 # @Email: 964028708@qq.com
 # @Date: 2024-07-11 14:18:06
 # @LastEditTime: 2025-11-27 11:41:57
 # @FilePath: /cy_frame/script/device/pull.sh
 # @Description: 更新脚本 - 调试用
 # @BugList: 
 # 
 # Copyright (c) 2024 by Cy, All Rights Reserved. 
 # 
### 

NAME=kk_frame
SPEED=38400

APP_DIR=/customer/app
LIB_DIR=/customer/lib

IPADDR=192.168.31.3
SERVEIP=192.168.31.2

killall $NAME
mount -o remount,rw /customer
ifconfig eth0 $IPADDR up

# 处理输入参数
for arg in "$@"; do
    case $arg in
        -app)
            cd $APP_DIR
            tftp -g -r $NAME $SERVEIP -b $SPEED
            chmod +x $NAME
            ;;
        -pak)
            cd $APP_DIR
            tftp -g -r $NAME.pak  $SERVEIP -b $SPEED
            ;;
        -cdroid)
            cd $APP_DIR
            tftp -g -r cdroid.pak  $SERVEIP -b $SPEED
            cd $LIB_DIR
            tftp -g -r libcdroid.so  $SERVEIP -b $SPEED
            ;;
        -tvhal)
            cd $LIB_DIR
            tftp -g -r libtvhal.so  $SERVEIP -b $SPEED
            ;;
        *)
            echo "Unknown arg: $arg"
            echo "Support args: -app -app -pak -cdroid -tvhal"
            ;;
    esac
done
