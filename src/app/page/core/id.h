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

// 页面定义
enum {
    PAGE_NULL,       // 空状态
    PAGE_DEMO,       // 示例页面
    PAGE_HOME,       // 主页面
    PAGE_OTA,        // OTA
};

// 弹窗定义
enum {
    POP_NULL,        // 空状态
    POP_LOCK,        // 童锁
    POP_TIP,         // 提示
};

// 语言定义
enum {
    LANG_ZH_CN,      // 简体中文
    LANG_ZH_TC,      // 繁体中文
    LANG_EN_US,      // 英文
    LANG_MAX,        // 语言数量
};

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
