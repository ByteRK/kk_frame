/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2026-03-22 12:49:51
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

int8_t DemoPage::getType() const {
    return PAGE_DEMO;
}

void DemoPage::setView() {
    click(AppRid::hello, [this](View& v) {
        g_window->showKeyboard("Factory", "Input Text~");
    });
    click(mRootView, [this](View& v) {
        bool isActivated = v.isActivated();
        v.setActivated(!isActivated);
        g_window->showToast(isActivated ? "Deactivated" : "Activated");
    });
}

void DemoPage::onTick() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick - g_window->mLastAction >= 120000) {
        if (tick - g_window->mLastAction <= 123000)
            g_window->removePop();
        g_window->showBlack();
    }
}

void DemoPage::onAttach() {
    g_window->setKeyboardCallBack([this](const std::string &text) {
        get<TextView>(AppRid::hello)->setText(text.empty() ? "Hello World" : text);
        if (text == "Factory") {
            g_windMgr->showPage(PAGE_FACTORY);
        }
    }, nullptr);
}

void DemoPage::onDetach() {
    g_window->setKeyboardCallBack(nullptr, nullptr);
}