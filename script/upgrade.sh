#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-09 09:38:07
 # @FilePath: /hana_frame/script/upgrade.sh
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 

# 返回值 
# 0:正常更新
# 1:未找到升级包
# 2:升级包解压失败

cd $(dirname $0)

PACK_DIR=/tmp                            # tar包的路径，默认为/tmp
FROM_USB=0                               # 是否从U盘升级，1表示从U盘升级，默认为0
UPDATING_SH="./updating.sh"              # U盘更新时运行的脚本
if [ ! -z "$1" ]; then
    PACK_DIR=$1
    # /vendor/udisk_sda1   U盘挂载目录
    if [ "${PACK_DIR:0:7}" == "/vendor" ]; then
        FROM_USB=1
        echo "wait aync usb disk"
        sleep 15
    fi

    if [ -d "$PACK_DIR" ]; then
        echo "The USB disk has been inserted"
        if [ -f "$UPDATING_SH" ]; then
            echo "have $UPDATING_SH"
            $UPDATING_SH &
        fi
    else
        echo "No USB disk : $PACK_DIR"
        exit 0
    fi
fi

NAME=hana_frame
BASE_NAME=customer

SRC_DIR=./                               # 更新资源路径（此变量无需手动更改）
TAR_DIR=/tmp                             # 升级包解压路径
DST_DIR=/$BASE_NAME                      # 新文件需要拷贝到的路径（即运行目录）
LOG_FILE=/appconfigs/upgrade.txt         # 程序LOG文件
UPD_FILE=/appconfigs/update.txt          # 更新日志
RUN_TIME=`date +'%Y-%m-%d %H:%M:%S'`     # 脚本运行时间

FIND_PACK=0                              # 是否找到升级包
UPD_COUNT=0                              # 更新的文件数量
STOP_APP=0                               # 停止FLAG

# 停止一些东西...
stopApp(){
    if [ $STOP_APP -eq 1 ]; then
        return
    fi
    STOP_APP=1
    # 日志文件
    if [ -f $LOG_FILE ] && [ -n "`du -sh $LOG_FILE|grep M`" ] ; then
        rm -rf $LOG_FILE
    fi
    # 开始更新
    echo 2 > $UPD_FILE
    sync
    sleep 3
    # 设置停止的FLAG
    echo "start update..." >> $LOG_FILE
}

# 更新文件
copyCustomer(){
    if [ ! -d $SRC_DIR ];then
        echo "path $SRC_DIR not exists"
        return 1
    fi

    # 重新mount,获得写权限
    mount -o remount,rw /$BASE_NAME

    cd $SRC_DIR
    for PATH_FILE in `find .` ; do
        if [ -d $PATH_FILE ] ; then
            continue
        fi
        src_md5=`md5sum $PATH_FILE|awk '{print $1}'`
        dst_file=$DST_DIR/$PATH_FILE

        # 检查升级包里的文件是否为软链接
        if [ -h $PATH_FILE ] ; then
            stopApp
            echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, file is a link, copy it."
            dst_dir=`dirname $dst_file`
            if [ ! -d $dst_dir ] ; then
                mkdir -p $dst_dir
            fi
            cp -af $PATH_FILE $dst_file
            UPD_COUNT=`expr $UPD_COUNT + 1`
            echo "$RUN_TIME add $SRC_DIR/$PATH_FILE" >> $LOG_FILE
            continue
        fi

        # 检测本地文件是否为目录或设备文件
        if [ ! -f $dst_file ]; then
            stopApp
            echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, dst file not exists, copy it."
            dst_dir=`dirname $dst_file`
            if [ ! -d $dst_dir ] ; then
                mkdir -p $dst_dir
            fi
            cp -af $PATH_FILE $dst_file
            UPD_COUNT=`expr $UPD_COUNT + 1`
            echo "$RUN_TIME add $SRC_DIR/$PATH_FILE" >> $LOG_FILE
        else
            # 检查本地文件是否为软链接
            if [ -h $dst_file ] ; then
                stopApp
                echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, dst_file is a link, replace it."
                rm -rf $dst_file
                cp -af $PATH_FILE $dst_file
                UPD_COUNT=`expr $UPD_COUNT + 1`
                echo "$RUN_TIME mod $SRC_DIR/$PATH_FILE" >> $LOG_FILE

                if [ -n "`echo $PATH_FILE | grep upgrade.sh`" ]; then
                    chmod 777 $dst_file
                fi
                continue
            fi

            # MD5校验
            dst_md5=`md5sum $dst_file|awk '{print $1}'`
            if [ "$src_md5" = "$dst_md5" ]; then
                echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, file md5[$dst_md5] is equal, skip it."
            else
                stopApp
                echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, file md5 not match, replace it."
                rm -rf $dst_file
                cp -af $PATH_FILE $dst_file
                UPD_COUNT=`expr $UPD_COUNT + 1`
                echo "$RUN_TIME mod $SRC_DIR/$PATH_FILE" >> $LOG_FILE

                if [ -n "`echo $PATH_FILE | grep upgrade.sh`" ]; then
                    chmod 777 $dst_file
                fi
            fi
        fi
    done
    cd -

    return 0
}

# 判断当前运行目录下是否存在BASE_NAME路径
if [ "$FROM_USB" -eq 0 &&  -d $BASE_NAME ]; then
    # 使用当前路径下的BASE_NAME目录
    SRC_DIR=`pwd`/$BASE_NAME
    FIND_PACK=1
else
    # 检查压缩包
    echo "Find updates from $PACK_DIR ..."
    for TAR_FILE in `find $PACK_DIR -name '*.tar.gz' | sort -r` ; do
        echo "[$TAR_FILE]checking necessary dir[$BASE_NAME] ..."

        #压缩包需包含customer文件
        FLAG=`tar -tvf $TAR_FILE | head -1 | grep -a "$BASE_NAME/"`
        if [ -z "$FLAG" ]; then
            echo "[$TAR_FILE] not find [$BASE_NAME], check next."
            continue
        fi

        # 解压到tmp目录
        echo "decompression $TAR_FILE"
        rm -rf $TAR_DIR/$BASE_NAME
        tar -zxf $TAR_FILE -C $TAR_DIR

        # 判断解压是否成功
        TAR_EXIT_CODE=$?
        if [ $TAR_EXIT_CODE -eq 0 ]; then
            echo "Tar extraction successful."
        else
            echo "Tar failed, Msy be $TAR_DIR not enough storage space. Exit code: $TAR_EXIT_CODE"
            echo "Tar failed, Msy be $TAR_DIR not enough storage space. Exit code: $TAR_EXIT_CODE" >> $LOG_FILE
            rm -rf $TAR_DIR/$BASE_NAME
            exit 2
        fi
        
        # 非U盘更新需将OTA文件删除
        if [ "$FROM_USB" -eq 0 ]; then
            rm -rf "$TAR_FILE"
        fi

        # 将更新的路径指向到解压路径
        SRC_DIR="$TAR_DIR/$BASE_NAME"
        FIND_PACK=1
        break
    done
fi

if [ "$FIND_PACK" -eq 0 ]; then
    echo "no updates found."
    echo "no updates found." >> "$LOG_FILE"
    exit 1
fi

# 备份原本程序
if [ "$FROM_USB" -eq 0 ]; then
    mount -o remount,rw /$BASE_NAME
    cp -f /customer/app/$NAME /customer/app/$NAME.bak
    sync
fi

# 同步文件
copyCustomer
if [ -n "`echo $SRC_DIR | grep tmp`" ]; then
    rm -rf $SRC_DIR
fi

# 结束标志
if [ $UPD_COUNT -gt 0 ]; then
    sync
    echo "update over, wait sync." >> $LOG_FILE
    sleep 10
    echo 1 > $UPD_FILE
    sync
else
    # U盘更新、无文件变动，无需重启
    if [ "$FROM_USB" -eq 1 ]; then
        echo "update nothing, no need to reboot."
        echo "update nothing, no need to reboot." >> $LOG_FILE
        exit 0
    fi
fi
reboot
