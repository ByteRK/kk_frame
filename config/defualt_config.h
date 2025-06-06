/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2025-05-28 17:07:34
 * @FilePath: /kk_frame/config/defualt_config.h
 * @Description: 项目信息
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __DEFUALT_CONFIG_H__
#define __DEFUALT_CONFIG_H__

/*********************** 文件信息 ***********************/

#ifndef CDROID_X64 // 本地文件保存路径
#define LOCAL_DATA_DIR "/appconfigs/"
#else
#define LOCAL_DATA_DIR "./apps/kk_frame/"
#endif

#define CONFIG_SECTION       "conf"                         // 配置文件节点

#define CONFIG_FILE_NAME     "config.xml"                   // 配置文件名
#define APP_FILE_NAME        "app.json"                     // 应用数据文件名

#define CONFIG_FILE_PATH  LOCAL_DATA_DIR CONFIG_FILE_NAME   // 配置文件完整路径
#define CONFIG_FILE_BAK_PATH CONFIG_FILE_PATH ".bak"        // 配置文件备份完整路径

#define APP_FILE_FULL_PATH    LOCAL_DATA_DIR APP_FILE_NAME  // 应用数据文件完整路径
#define APP_FILE_BAK_PATH APP_FILE_FULL_PATH ".bak"         // 应用数据文件备份完整路径

#define APP_FIRST_INIT_TAG   "./FIRSTINIT.TAG"              // 第一次初始化标记

/*********************** 默认设置 ***********************/

// 屏幕亮度
#define CONFIG_BRIGHTNESS     80       // 默认亮度
#define CONFIG_BRIGHTNESS_MIN 0        // 最小亮度

#define CONFIG_VOLUME         80       // 音量
#define CONFIG_AUTOLOCK       false    // 自动锁屏

#endif