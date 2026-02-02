/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2026-02-02 16:49:24
 * @FilePath: /kk_frame/config/config_info.h
 * @Description: 项目信息
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CONFIG_INFO_H__
#define __CONFIG_INFO_H__

/*********************** 文件信息 ***********************/
#if defined(PRODUCT_SIGMA)
#define LOCAL_DATA_DIR "/appconfigs/"
#elif defined(PRODUCT_RK3506)
#define LOCAL_DATA_DIR "/userdata/cdroid/app/data/"
#else
#include "app_version.h"
#define LOCAL_DATA_DIR "./apps/" APP_NAME_STR "/"
#endif
#define DEFINE_FILE_INFO(name, file) \
    static const char* name##_FILE_NAME = #file; \
    static const char* name##_FILE_PATH = LOCAL_DATA_DIR #file; \
    static const char* name##_FILE_BAK_PATH = LOCAL_DATA_DIR #file ".bak";

DEFINE_FILE_INFO(APP,        app.json);           // 应用数据文件
DEFINE_FILE_INFO(CONFIG,     config.xml);         // 配置文件名
DEFINE_FILE_INFO(HISTORY,    history.json)        // 历史记录文件名
DEFINE_FILE_INFO(STATISTICS, statistics.json)     // 统计文件名

#define APP_FIRST_INIT_TAG "./FIRSTINIT.TAG"  // 第一次初始化标记

/*********************** 默认设置 ***********************/

#define CONFIG_BRIGHTNESS     80       // 亮度
#define CONFIG_VOLUME         80       // 音量
#define CONFIG_WIFI           false    // wifi
#define CONFIG_WIFI_SSID      "Ricken" // wifi SSID
#define CONFIG_WIFI_PASSWORD  "Ricken" // wifi 密码
#define CONFIG_AUTOLOCK       false    // 自动锁屏

#endif // __CONFIG_INFO_H__