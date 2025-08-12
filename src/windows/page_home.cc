/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2025-08-10 18:13:34
 * @FilePath: /kk_frame/src/windows/page_home.cc
 * @Description: 主页面
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */


#include "page_home.h"
#include "wind_mgr.h"

HomePage::HomePage() :PageBase("@layout/page_home") {
    initUI();
}

HomePage::~HomePage() {
}

void HomePage::onTick() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick - g_window->mLastAction >= 120000) {
        if (tick - g_window->mLastAction <= 123000)
            g_window->removePop();
        g_window->showBlack();
    }
}

uint8_t HomePage::getType() const {
    return PAGE_HOME;
}

void HomePage::setView() {
    mRootView->setOnClickListener([](View&){LOGE("HELLO WORLD!!!");});
}