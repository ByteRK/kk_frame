#!/bin/sh
###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-07-02 17:11:30
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
MAX_RETRIES=3                            # 最大重试次数
USB_MOUNT_PREFIX="/vendor/udisk"         # U盘挂载目录前缀

NAME=hana_frame
BASE_NAME=customer
BAK_DIR=/customer/app/bakFiles

SRC_DIR=./                               # 更新资源路径（此变量无需手动更改）
TAR_DIR=/tmp                             # 升级包解压路径
DST_DIR=/$BASE_NAME                      # 新文件需要拷贝到的路径（即运行目录）
LOG_FILE=/appconfigs/upgrade.txt         # 程序LOG文件
UPD_FILE=/appconfigs/update.txt          # 更新日志
RUN_TIME=`date +'%Y-%m-%d %H:%M:%S'`     # 脚本运行时间

FIND_PACK=0                              # 是否找到升级包
UPD_COUNT=0                              # 更新的文件数量
STOP_APP=0                               # 停止FLAG

# 检测所有可能的U盘分区
detect_usb_partitions() {
    partitions=""
    for dev in /sys/block/sd*; do
        if [ -e "$dev/device" ]; then
            for part in "$dev"/sd*[0-9]; do
                part_name=$(basename "$part")
                mount_point="${USB_MOUNT_PREFIX}_${part_name}"
                partitions="$partitions $mount_point"
            done
        fi
    done
    echo "$partitions"
}

# 函数：验证tar.gz是否有效更新包
validate_update_pkg() {
    local tar_file="$1"
    echo "[$(date)] Checking $tar_file" | tee -a "$LOG_FILE"
    
    # 检查是否包含BASE_NAME目录
    if ! tar -tzf "$tar_file" | grep -q "^$BASE_NAME/"; then
        echo "Invalid package: $tar_file (missing $BASE_NAME directory)" | tee -a "$LOG_FILE"
        return 1
    fi
    
    # 检查是否包含至少一个文件
    if ! tar -tzf "$tar_file" | grep -q "^$BASE_NAME/.*"; then
        echo "Invalid package: $tar_file (empty $BASE_NAME directory)" | tee -a "$LOG_FILE"
        return 1
    fi
    
    return 0
}

# 函数：解压更新包
extract_package() {
    local tar_file="$1"
    local retry=0
    
    while [ $retry -lt $MAX_RETRIES ]; do
        echo "Extracting $tar_file (attempt $((retry+1)))..." | tee -a "$LOG_FILE"
        rm -rf "$TAR_DIR/$BASE_NAME"
        
        if tar -zxf "$tar_file" -C "$TAR_DIR"; then
            # 二次验证解压结果
            if [ -d "$TAR_DIR/$BASE_NAME" ]; then
                echo "Extraction successful" | tee -a "$LOG_FILE"
                return 0
            fi
        fi
        
        ((retry++))
        sleep 1
    done
    
    echo "Failed to extract $tar_file after $MAX_RETRIES attempts" | tee -a "$LOG_FILE"
    return 1
}

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

bakFiles(){
    local PATH_FILE=$1

    if [ ! -d $BAK_DIR ] ; then
        mkdir -p $BAK_DIR
    fi

    local dst_dir=`dirname $BAK_DIR/$PATH_FILE`
    if [ ! -d $dst_dir ] ; then
        mkdir -p $dst_dir
    fi

    if [ -h "$BAK_DIR/$PATH_FILE" ]; then
        echo "File $PATH_FILE is a link in $BAK_DIR. Skipping..."
    elif [ -f "$BAK_DIR/$PATH_FILE" ]; then
        echo "File $PATH_FILE already exists in $BAK_DIR. Skipping..."
    elif [ -h "$DST_DIR/$PATH_FILE" ]; then
        cp -af "$DST_DIR/$PATH_FILE" "$BAK_DIR/$PATH_FILE"
        echo "Bak File is a link $PATH_FILE copy in $BAK_DIR."
    else
        mv "$DST_DIR/$PATH_FILE" "$BAK_DIR/$PATH_FILE"
        echo "Bak File $PATH_FILE move in $BAK_DIR."
    fi

}

# 更新文件
copyCustomer(){
    if [ ! -d $SRC_DIR ];then
        echo "path $SRC_DIR not exists"
        return 1
    fi

    # 重新mount,获得写权限
    mount -o remount,rw /$BASE_NAME
    
    # 计算当前更新包的 MD5（假设更新包在 $SRC_DIR 下）
    CURRENT_MD5=$(find "$SRC_DIR" -type f -exec md5sum {} + | sort | md5sum | cut -d' ' -f1)
    
    # 读取上次保存的 MD5（如果存在）
    LAST_MD5_FILE="/customer/app/last_update_md5.txt"
    if [ -f "$LAST_MD5_FILE" ]; then
        LAST_MD5=$(cat "$LAST_MD5_FILE")
    else
        LAST_MD5=""
    fi

    # 如果 MD5 相同，跳过删除备份
    if [ "$CURRENT_MD5" = "$LAST_MD5" ]; then
        echo "Update package MD5 unchanged. Skipping backup deletion."
    else
        # MD5 不同，记录新 MD5 并删除旧备份
        echo "$CURRENT_MD5" > "$LAST_MD5_FILE"
        if [ -d "$BAK_DIR" ]; then
            rm -rf "$BAK_DIR"
            sync
            sleep 2
        fi
    fi

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
            bakFiles "$PATH_FILE"
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
            bakFiles "$PATH_FILE"
            cp -af $PATH_FILE $dst_file
            UPD_COUNT=`expr $UPD_COUNT + 1`
            echo "$RUN_TIME add $SRC_DIR/$PATH_FILE" >> $LOG_FILE
        else
            # 检查本地文件是否为软链接
            if [ -h $dst_file ] ; then
                stopApp
                echo "check file. src=$SRC_DIR/$PATH_FILE dst=$dst_file, dst_file is a link, replace it."
                bakFiles "$PATH_FILE"
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
                bakFiles "$PATH_FILE"
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

# 主逻辑
if [ "$FROM_USB" -eq 0 ] && [ -d "$BASE_NAME" ]; then
    # 使用当前目录下的更新 
    SRC_DIR=$(pwd)/"$BASE_NAME"
    FIND_PACK=1
else
    sleep 10
    # 检查U盘分区
    usb_partitions=$(detect_usb_partitions)

    # 指定一个变量来跟踪是否找到更新包
    found_update_package=0

    for mount_point in $usb_partitions; do
        FROM_USB=1
        echo "Searching for update packages in $mount_point..." | tee -a "$LOG_FILE"
        
       # 使用简单的 find 查找 tar.gz 文件
        TAR_FILE=$(find "$mount_point" -name '*.tar.gz' | sort | tail -n 1)
        
        if [ -n "$TAR_FILE" ]; then
            echo "Found package: $TAR_FILE" | tee -a "$LOG_FILE"
            
            # 验证包有效性
            if ! validate_update_pkg "$TAR_FILE"; then
                continue
            fi
            
            # 解压包
            if extract_package "$TAR_FILE"; then
                SRC_DIR="$TAR_DIR/$BASE_NAME"
                FIND_PACK=1
                found_update_package=1
                break  # 退出循环
            fi
        else
            echo "No .tar.gz files found in $mount_point." | tee -a "$LOG_FILE"
        fi
        # 如果在U盘未找到更新包，尝试查找压缩包
        if [ "$found_update_package" -eq 0 ]; then
            echo "Finding updates from $PACK_DIR ..." | tee -a "$LOG_FILE"
            for TAR_FILE in $(find "$PACK_DIR" -name '*.tar.gz' | sort -r); do
                echo "[$TAR_FILE] checking necessary dir [$BASE_NAME] ..."

                # 压缩包需包含BASE_NAME目录
                if ! tar -tvf "$TAR_FILE" | grep -q "$BASE_NAME/"; then
                    echo "[$TAR_FILE] not find [$BASE_NAME], check next." | tee -a "$LOG_FILE"
                    continue
                fi

                # 解压到tmp目录
                echo "Decompressing $TAR_FILE..." | tee -a "$LOG_FILE"
                rm -rf "$TAR_DIR/$BASE_NAME"
                tar -zxf "$TAR_FILE" -C "$TAR_DIR"

                # 判断解压是否成功
                if [ $? -eq 0 ]; then
                    echo "Tar extraction successful." | tee -a "$LOG_FILE"
                    SRC_DIR="$TAR_DIR/$BASE_NAME"
                    FIND_PACK=1
                    break  # 找到并解压后退出循环
                else
                    echo "Tar failed, maybe $TAR_DIR not enough storage space." | tee -a "$LOG_FILE"
                    exit 2
                fi

                # 非U盘更新需将OTA文件删除
                [ "$FROM_USB" -eq 0 ] && rm -rf "$TAR_FILE"
            done
        fi
    done

    # 如果未找到更新包，则输出相应信息
    if [ "$found_update_package" -eq 0 ]; then
        echo "No updates found in any USB partitions." | tee -a "$LOG_FILE"
        exit 1
    fi
fi
    

if [ "$FIND_PACK" -eq 0 ]; then
    echo "no updates found."
    echo "no updates found." >> "$LOG_FILE"
    exit 1
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
