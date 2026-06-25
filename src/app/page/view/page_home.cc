/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2026-06-25 11:45:23
 * @FilePath: /kk_frame/src/app/page/view/page_home.cc
 * @Description: 主页面
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "page_home.h"
#include "wind_mgr.h"

PAGE_REGISTER(PAGE_HOME, HomePage);

HomePage::HomePage() :PageBase("@layout/page_home") {
    initUI();
}

HomePage::~HomePage() { }

void HomePage::onTick(int64_t now) {
}

int8_t HomePage::getType() const {
    return PAGE_HOME;
}

void HomePage::onAttach() {
    startTick();
}

void HomePage::onDetach() {
    stopTick();
}

void HomePage::setView() {
}
