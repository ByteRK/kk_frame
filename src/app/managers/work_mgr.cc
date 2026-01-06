/*
 * Author: Ricken
 * Email: me@ricken.cn
 * Date: 2026-01-06 14:53:24
 * LastEditTime: 2026-01-06 14:58:14
 * FilePath: /kk_frame/src/app/managers/work_mgr.cc
 * Description: 工作状态管理
 * BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "work_mgr.h"
#include <cdlog.h>

WorkMgr::WorkMgr() {
}

WorkMgr::~WorkMgr() {
}

void WorkMgr::init() {
}

void WorkMgr::onTimer(uint32_t id, size_t param, uint32_t count) {
    if (param == 1) {
        mXXXOverTimer -= 1;
        if (mXXXOverTimer <= 0)
            stopXXX();
        LOGI("XXX Working: %d/%d", mXXXOverTimer, mXXXOverTimer + count);
    }
}

void WorkMgr::startXXX() {
    LOGI("start XXX Work");
    mXXXOverTimer = 1000;
    g_timer->addTimer(1000, this, 1, 0);
}

void WorkMgr::stopXXX() {
}
