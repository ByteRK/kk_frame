/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2025-11-27 11:45:21
 * @FilePath: /cy_frame/src/windows/page_home.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
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

int8_t HomePage::getType() const {
    return PAGE_HOME;
}

void HomePage::setView() {
    mRootView->setOnClickListener([](View&){LOGE("HELLO WORLD!!!");});
}