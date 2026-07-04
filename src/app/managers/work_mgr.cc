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

void WorkMgr::startXXX() {
    LOGI("start XXX Work");
    mXXXOverTimer = 1000;
    mXXXTimer = g_timer->scopedRepeated(1000, 1000,
        [this](TimerMgr::TimerId, uint32_t count) {
        mXXXOverTimer -= 1;
        if (mXXXOverTimer <= 0) {
            stopXXX();
        }
        LOGI("XXX Working: %d/%d", mXXXOverTimer, mXXXOverTimer + count);
    });
}

void WorkMgr::stopXXX() {
    mXXXTimer.cancel();
}
