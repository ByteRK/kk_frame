/*
 * 项目信息
 * 资源路径以及默认配置
 *
**/

#ifndef __CONFIG_INFO_H__
#define __CONFIG_INFO_H__

/*********************** 基础路径 ***********************/
#if defined(PRODUCT_SIGMA)
#define APP_RW_DIR     "/appconfigs/"
#define APP_R_ONLY_DIR "/appconfigs/"
#define APP_RES_DIR    "./res/"
#define APP_OTA_DIR    "./ota/"
#elif defined(PRODUCT_RK3506)
#define APP_RW_DIR     "/userdata/app/"
#define APP_R_ONLY_DIR "/userdata/"
#define APP_RES_DIR    "/userdata/app/res/"
#define APP_OTA_DIR    "/userdata/app/ota/"
#else
#include "app_version.h"
#define APP_RW_DIR     "./apps/"  APP_NAME_STR "/"       // 应用读写目录
#define APP_R_ONLY_DIR "./apps/"  APP_NAME_STR "/"       // 应用只读目录
#define APP_RES_DIR    "../apps/" APP_NAME_STR "/res/"   // 应用资源目录
#define APP_OTA_DIR    "./apps/"  APP_NAME_STR "/ota/"   // 应用更新目录
#endif

#define APP_DATA_DIR   APP_RW_DIR  "data/"               // 应用运行数据目录
#define APP_FIRST_TAG  APP_RW_DIR  "APP_FIRST.TAG"       // 首次初始化完成标记

/*********************** 文件信息 ***********************/
// 设备信息宏
#define DEFINE_DEV_FILE_INFO(name, file) \
    static constexpr const char* name##_FILE_NAME = #file; \
    static constexpr const char* name##_FILE_PATH = APP_R_ONLY_DIR #file; \
    static constexpr const char* name##_FILE_BAK_PATH = APP_R_ONLY_DIR #file ".bak";

// 应用数据宏
#define DEFINE_DATA_FILE_INFO(name, file) \
    static constexpr const char* name##_FILE_NAME = #file; \
    static constexpr const char* name##_FILE_PATH = APP_DATA_DIR #file; \
    static constexpr const char* name##_FILE_BAK_PATH = APP_DATA_DIR #file ".bak";

DEFINE_DEV_FILE_INFO(DEVCONF,     devices.xml)         // 设备配置文件

DEFINE_DATA_FILE_INFO(APP,        app.json);           // 应用数据文件
DEFINE_DATA_FILE_INFO(CONFIG,     config.xml);         // 配置文件名
DEFINE_DATA_FILE_INFO(WIFI,       wifi.json);          // WIFI配置文件名
DEFINE_DATA_FILE_INFO(HISTORY,    history.json)        // 历史记录文件名
DEFINE_DATA_FILE_INFO(STATISTICS, statistics.json)     // 统计文件名

/*********************** 资源信息 ***********************/

#define RES_WEATHER_DIR  APP_RES_DIR  "weather/"       // 天气资源路径

/*********************** 默认设置 ***********************/
#define WIFI_SWITCH           false    // wifi
#define WIFI_SSID             ""       // wifi SSID
#define WIFI_PASSWORD         ""       // wifi 密码

#define CONFIG_BRIGHTNESS     80       // 亮度
#define CONFIG_VOLUME         80       // 音量
#define CONFIG_SCREEN_SAVE    120      // 屏保时间
#define CONFIG_AUTOLOCK       false    // 自动锁屏

#endif // __CONFIG_INFO_H__
