/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2026-01-29 10:23:55
 * @FilePath: /kk_frame/src/app/page/view/page_demo.cc
 * @Description: 框架演示主页面（建议保留）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "page_demo.h"
#include "wind_mgr.h"

PAGE_REGISTER(PAGE_DEMO, DemoPage);

DemoPage::DemoPage() :PageBase("@layout/page_demo") {
    initUI();
}

DemoPage::~DemoPage() {
}

void DemoPage::onTick() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick - g_window->mLastAction >= 120000) {
        if (tick - g_window->mLastAction <= 123000)
            g_window->removePop();
        g_window->showBlack();
    }
}

int8_t DemoPage::getType() const {
    return PAGE_DEMO;
}

void DemoPage::setView() {
    click(AppRid::hello, [](View& v) {
        v.setActivated(!v.isActivated());
        g_window->showToast("STOP !!!");
    });
}