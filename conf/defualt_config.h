#ifndef __DEFUALT_CONFIG_H__
#define __DEFUALT_CONFIG_H__

/*********************** 文件信息 ***********************/

#define CONFIG_SECTION  "conf"

#ifndef CDROID_X64 // 本地文件保存路径
#define LOCAL_DATA_DIR "/appconfigs/"
#else
#define LOCAL_DATA_DIR "./"
#endif

#ifndef CDROID_X64 // 配置文件名
#define CONFIG_FILE_NAME     "config.xml"
#define APP_FILE_NAME        "app.json"
#else
#define CONFIG_FILE_NAME     "kk_frame3_config.xml"
#define APP_FILE_NAME        "kk_frame3_app.json"
#endif


#define CONFIG_FILE_PATH  LOCAL_DATA_DIR CONFIG_FILE_NAME // 配置文件完整路径
#define CONFIG_FILE_BAK_PATH   CONFIG_FILE_PATH ".bak"    // 配置文件备份完整路径

#define APP_FILE_FULL_PATH    LOCAL_DATA_DIR APP_FILE_NAME
#define APP_FILE_BAK_PATH     APP_FILE_FULL_PATH ".bak"


/*********************** 默认设置 ***********************/

// MCU类型
#define  HV_SYS_CONFIG_MCU 212

// 屏幕亮度
#define  HV_SYS_CONFIG_SCREEN_BRIGHTNESS   50
#define  HV_SYS_CONFIG_MAX_SCREEN_BRIGHTNESS   100

#endif