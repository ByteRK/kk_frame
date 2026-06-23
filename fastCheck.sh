###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2026-06-17 01:21:46
 # @LastEditTime: 2026-06-17 01:26:47
 # @FilePath: /kk_frame/fastCheck.sh
 # @Description: 快速编译检验脚本
 # @BugList: 
 # 
 # Copyright (c) 2026 by Ricken, All Rights Reserved. 
 # 
### 

#!/bin/bash

# 切换到当前脚本所在目录
cd $(dirname $0)

# 项目参数
NAME=kk_frame

# 编译参数
PRODUCT=x64
PRODUCT_DIR=outX64-Debug

# 构建相关路径
SRC_DIR="$(realpath "$(pwd)")"
PACKAGE_DIR=$SRC_DIR/dist
CDROID_DIR="$(realpath "$SRC_DIR/../..")"
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

# 编译
cd $OUT_DIR
make $NAME -j8

# 拷贝文件
cp $OUT_DIR/apps/$NAME/$NAME*               $PACKAGE_DIR/
cp $OUT_DIR/src/gui/cdroid.pak                 $PACKAGE_DIR/
cp $OUT_DIR/src/gui/libcdroid.so               $PACKAGE_DIR/
cp $OUT_DIR/src/porting/$PRODUCT/libtvhal.so   $PACKAGE_DIR/
chmod +x $PACKAGE_DIR/$NAME
