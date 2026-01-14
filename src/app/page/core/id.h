/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Description: 相关ID定义
 *               页面、弹窗、语言
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __BASE_ID_H__
#define __BASE_ID_H__

#include <string>

/// @brief 页面类型
typedef enum {
    PAGE_NULL,         // 空状态
    PAGE_DEMO,         // 示例页面（自动进入）
    PAGE_HOME,         // 主页面
    PAGE_OTA,          // OTA

    PAGE_MAX,          // 页面类型数量
} PAGE_TYPE;

/// @brief 弹窗类型
typedef enum {
    POP_NULL,          // 空状态
    POP_LOCK,          // 童锁
    POP_TIP,           // 提示
    
    POP_MAX,           // 弹窗类型数量
} POP_TYPE;

/// @brief 消息类型
typedef enum {
    MSG_GENERAL = 0,   // 通用消息

    MSG_MAX,           // 消息类型数量
} MSG_TYPE;

// 语言定义
typedef enum {
    LANG_ZH_CN,        // 简体中文
    LANG_ZH_TC,        // 繁体中文
    LANG_EN_US,        // 英文

    LANG_MAX,          // 语言类型数量
} LANG_TYPE;

/// @brief 语言转文本
/// @param lang 语言
/// @return 
static std::string langToText(int lang) {
    const static char* langText[] = {
        "中文", "繁体", "English"
    };
    return langText[lang % LANG_MAX];
};

#endif
