#!/bin/sh
#set -x

###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-05-22 15:42:58
 # @LastEditTime: 2025-01-15 23:30:25
 # @FilePath: /kk_frame/script/build.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

NAME=kk_frame
SRC_DIR=$NAME
CDROID_DIR=~/cdroid
PRODUCT=sigma
PRODUCT_DIR=outSIGMA-Release

APP_DIR=$CDROID_DIR/apps/$SRC_DIR
APP_OUT_DIR=$CDROID_DIR/$PRODUCT_DIR/apps/$SRC_DIR

if [ "$1" = "-t" ]; then
    touch $APP_DIR/CMakeLists.txt
fi

if [ "$1" = "-rm" ]; then
    cd $CDROID_DIR
    rm -rf ./$PRODUCT_DIR
    ./build.sh --product=$PRODUCT
fi

if [ ! -d "$APP_DIR/package" ]; then
    mkdir $APP_DIR/package
fi

if [ ! -d "$APP_DIR/package/lib" ]; then
    mkdir $APP_DIR/package/lib
fi

cd $CDROID_DIR/$PRODUCT_DIR/
make $NAME -j33
chmod +x $APP_OUT_DIR/$NAME

cp $APP_OUT_DIR/$SRC_DIR/$NAME $APP_DIR/package/
cp $APP_OUT_DIR/$SRC_DIR/$NAME.pak $APP_DIR/package/
cp $CDROID_DIR/$PRODUCT_DIR/src/gui/cdroid.pak $APP_DIR/package/
cp $CDROID_DIR/$PRODUCT_DIR/src/gui/libcdroid.so $APP_DIR/package/lib/