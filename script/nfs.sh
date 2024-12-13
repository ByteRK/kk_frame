#!/bin/sh
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2024-05-22 15:42:58
 # @LastEditTime: 2024-12-13 15:59:55
 # @FilePath: /kk_frame/script/nfs.sh
 # @Description: 
 # @BugList: 
 # 
 # Copyright (c) 2024 by Ricken, All Rights Reserved. 
 # 
### 

ID=62367
NAME=kk_frame
SOC=ssd210
IMAGE=ssd210/sat62367m

if [ "$1" = "-p" ]; then
    cp ~/cdroid/outSIGMA-Release/apps/$NAME/$NAME* ~/image/$IMAGE/sysfs/customer/app
    cp ~/cdroid/outSIGMA-Release/src/gui/cdroid.pak ~/image/$IMAGE/sysfs/customer/app
    cp ~/cdroid/outSIGMA-Release/src/gui/libcdroid.so ~/image/$IMAGE/sysfs/customer/lib
    chmod +x ~/image/$IMAGE/sysfs/customer/app/$NAME

    cd ~/image/$SOC/
    rm -rf ./images
    echo "------------------- NEXT -------------------"
#   echo "export PATH=$PATH:/opt/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/"
    echo "export PATH=\$PATH:/opt/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/"
    echo "./build.sh"
    echo "zip ./images.zip -r ./images/*"
    echo "--------------------------------------------"
    exit
fi

if [ "$1" = "-t" ]; then
    touch ~/cdroid/apps/$NAME/CMakeLists.txt
fi

if [ "$1" = "-rm" ]; then
    cd ~/cdroid/
    rm -rf ./outSIGMA-Release
    ./build.sh --product=sigma
fi

cd ~/cdroid/outSIGMA-Release/
make $NAME -j33
cp ~/cdroid/outSIGMA-Release/apps/$NAME/$NAME* /home/nfs/$ID/
cp ~/cdroid/outSIGMA-Release/src/gui/cdroid.pak /home/nfs/$ID/
cp ~/cdroid/outSIGMA-Release/src/gui/libcdroid.so /home/nfs/$ID/lib/
chmod +x /home/nfs/$ID/$NAME

cp /home/nfs/$ID/$NAME ~/cdroid/apps/$NAME/package/
cp /home/nfs/$ID/$NAME.pak ~/cdroid/apps/$NAME/package/
cp /home/nfs/$ID/cdroid.pak ~/cdroid/apps/$NAME/package/
cp /home/nfs/$ID/lib/libcdroid.so ~/cdroid/apps/$NAME/package/