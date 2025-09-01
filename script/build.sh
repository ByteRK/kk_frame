#!/bin/sh
#set -x

###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-09-01 16:39:24
 # @FilePath: /hana_frame/script/build.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 

# 切换到当前脚本所在目录
cd $(dirname $0)

# 项目参数
NAME=hana_frame
SRC_DIR=$NAME
DIRNAME=$NAME

# 编译参数
PRODUCT=sigma
PRODUCT_DIR=outSIGMA-Release

# 构建相关路径
SRC_DIR=$(pwd)/..
PACKAGE_DIR=$SRC_DIR/package
CDROID_DIR=$SRC_DIR/../..
OUT_DIR=$CDROID_DIR/$PRODUCT_DIR

# 处理输入参数
for arg in "$@"; do
    case $arg in
        -t)
            touch $SRC_DIR/CMakeLists.txt
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

# 检查编译目录是否存在
if [ ! -d "$OUT_DIR" ]; then
    cd $CDROID_DIR
    ./build.sh --product=$PRODUCT
    if [ ! -d "$OUT_DIR" ]; then
        echo "Cannot make out dir: $PRODUCT_DIR"
        exit 1
    fi
fi

# 检查输出目录是否存在
if [ ! -d "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    if [ ! -d "$PACKAGE_DIR" ]; then
        echo "Cannot make package dir: $PACKAGE_DIR"
        exit 1
    fi
fi

# 生成版本号 ⚠️新版本不用，已在CMakeLists.txt中设置自动生成
# cd $SRC_DIR
# chmod +x ./date2ver
# ./date2ver

# 编译
cd $OUT_DIR
make $NAME -j8

# 拷贝文件
cp $OUT_DIR/apps/$DIRNAME/$NAME*               $PACKAGE_DIR/
cp $OUT_DIR/src/gui/cdroid.pak                 $PACKAGE_DIR/
cp $OUT_DIR/src/gui/libcdroid.so               $PACKAGE_DIR/
cp $OUT_DIR/src/porting/$PRODUCT/libtvhal.so   $PACKAGE_DIR/
chmod +x $PACKAGE_DIR/$NAME