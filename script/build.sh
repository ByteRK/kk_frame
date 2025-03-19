#!/bin/sh
#set -x

###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-05-22 15:42:58
 # @LastEditTime: 2025-01-15 23:35:48
 # @FilePath: /kk_frame/script/build.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

# 项目参数
NAME=kk_frame
SRC_DIR=$NAME
CDROID_DIR=~/cdroid
PRODUCT=sigma
PRODUCT_DIR=outSIGMA-Release

# 构建相关路径
APP_DIR=$CDROID_DIR/apps/$SRC_DIR
APP_OUT_DIR=$CDROID_DIR/$PRODUCT_DIR/apps/$SRC_DIR

# 处理输入参数
for arg in "$@"; do
    case $arg in
        -t)
            touch $APP_DIR/CMakeLists.txt
            ;;
        -rm)
            cd $CDROID_DIR
            rm -rf ./$PRODUCT_DIR
            ./build.sh --product=$PRODUCT
            ;;
        *)
            echo "未知参数: $arg"
            exit 1
            ;;
    esac
done

# 检查package目录
if [ ! -d "$APP_DIR/package" ]; then
    mkdir -p "$APP_DIR/package"
fi
if [ ! -d "$APP_DIR/package/lib" ]; then
    mkdir -p "$APP_DIR/package/lib"
fi

# 生成版本号
cd $APP_DIR
chmod +x ./date2ver
./date2ver

# 编译
cd $CDROID_DIR/$PRODUCT_DIR
make $NAME -j33
chmod +x $APP_OUT_DIR/$NAME

# 复制文件
cp $APP_OUT_DIR/$SRC_DIR/$NAME $APP_DIR/package/
cp $APP_OUT_DIR/$SRC_DIR/$NAME.pak $APP_DIR/package/
cp $CDROID_DIR/$PRODUCT_DIR/src/gui/cdroid.pak $APP_DIR/package/
cp $CDROID_DIR/$PRODUCT_DIR/src/gui/libcdroid.so $APP_DIR/package/lib/
