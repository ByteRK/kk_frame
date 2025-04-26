###
 # @Author: cy
 # @Email: 964028708@qq.com
 # @Date: 2025-04-26 15:49:45
 # @LastEditTime: 2025-04-26 16:20:32
 # @FilePath: /cy_frame/script/start.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2025 by cy, All Rights Reserved. 
 # 
### 

export LD_LIBRARY_PATH=/customer/lib:$LD_LIBRARY_PATH:/config/wifi
export FONTCONFIG_PATH=/customer/fonts/

#./kaidu_ms7 --rotate=270 &
#exit
(
count=0
runCount=0
while true; do
    # 判断是否拉起20次程序，如果拉起20次程序，则把备份文件替换过来，并重启系统
    if [ $count -gt 20 ]; then
        mount -o remount,rw /customer
        cp /customer/app/bakFiles/* -ra /customer/
        reboot
    elif [ ! -d "/tmp/customer" ] &&  ! pgrep -f './kaidu_ms7' > /dev/null; then
        current_time=$(date "+%Y-%m-%d %H:%M:%S")
        count=$(expr $count + 1);
        #echo -e "\033[33m $current_time.$milliseconds : APP RESTART \033[0m"
        ./kaidu_ms7 --rotate=270 --resetCount=$count  >/dev/null 2>/dev/null  &
        runCount=0
    fi
    sleep 3
done
)&

