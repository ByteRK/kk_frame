#!/bin/sh
#set -x

# 参数定义
NAME=kk_frame
CDROID_DIR=~/cdroid
OUTX64_DIR=./outX64-Debug
RUN_CMD=./apps/$NAME/$NAME

# 切换到CDROID目录
cd $CDROID_DIR

# 处理输入参数
for arg in "$@"; do
    case $arg in
        -t)
            touch ./apps/$NAME/CMakeLists.txt
            ;;
        -rm)
            rm -rf $OUTX64_DIR
            ;;
        -gdb)
            RUN_CMD="gdb $RUN_CMD"
            ;;
        -conn)
            RUN_CMD="$RUN_CMD conn_mgr.cc:v"
            ;;
        -debug)
            RUN_CMD="$RUN_CMD --debug"
            ;;
        *)
            echo "未知参数: $arg"
            exit 1
            ;;
    esac
done

# 判断是否需要build
if [ ! -d "$OUTX64_DIR" ]; then
    ./build.sh --build=debug
fi

# 切换到输出目录
cd $OUTX64_DIR

# 编译程序
make $NAME -j3

# 判断程序资源
if [ ! -f "./$NAME.pak" ]; then
    ln -s ./apps/$NAME/$NAME.pak ./$NAME.pak
fi

# 运行程序
$RUN_CMD
