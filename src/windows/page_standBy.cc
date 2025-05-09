/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:12:25
 * @FilePath: /hana_frame/src/windows/page_standBy.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */


#include "page_standBy.h"
#include "manage.h"

StandByPage::StandByPage() :PageBase("@layout/page_standby") {
    initUI();
}

StandByPage::~StandByPage() {
}

void StandByPage::onTick() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick - g_window->mLastAction >= 120000) {
        if (tick - g_window->mLastAction <= 123000)
            g_window->removePop();
        g_window->showBlack();
    }
}

uint8_t StandByPage::getType() const {
    return PAGE_STANDBY;
}

void StandByPage::setView() {
    mRootView->setOnClickListener([](View&){LOGE("HELLO WORLD!!!");});
}