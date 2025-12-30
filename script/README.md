### 目录说明

该目录下存放的是一些常用的脚本文件，用于自动化处理一些任务。
包括但不限于：构建、补丁、语言检查、设备管理、字体管理、OTA更新等。

### 备注
~~~
./script/
├── build.sh                构建升级文件
├── cdroid_patch.sh         打包当前cdroid框架修改内容
├── checkLang.py            检查多语言配置文件是否匹配
├── fill_bg.py              填充PNG图片背景色
├── fonts.sh                编译时自动生成字体相关文件
├── kk_frame.ps1            用于在Windows系统下创建AS项目软链到服务器
├── ota.py                  用于给OTA包插入MD5信息头
├── x64.sh                  x64环境下的一些操作集合
└── device
    ├── env.sh              用于设置设备环境变量
    ├── mount.sh            用于挂载设备
    ├── pull.sh             用于从设备拉取文件
    ├── run.sh              用于在设备上运行程序
    ├── updating.sh         用于更新设备
    └── upgrade.sh          用于升级设备
~~~
