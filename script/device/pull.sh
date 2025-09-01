###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-09-01 16:39:45
 # @FilePath: /hana_frame/script/pull.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 

NAME=hana_frame
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
