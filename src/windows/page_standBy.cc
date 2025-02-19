/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2025-02-20 02:08:48
 * @FilePath: /kk_frame/src/windows/page_standBy.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
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