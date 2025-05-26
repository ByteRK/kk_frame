/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-26 10:14:36
 * @FilePath: /hana_frame/src/windows/page_home.cc
 * @Description:
 * Copyright (c) 2025 by hanakami, All Rights Reserved.
 */


#include "page_home.h"
#include "manage.h"

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
    mRootView->setOnClickListener([](View&) {LOGE("HELLO WORLD!!!");});
}