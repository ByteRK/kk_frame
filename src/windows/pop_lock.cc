/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-02-13 16:26:20
 * @LastEditTime: 2025-11-27 11:45:28
 * @FilePath: /cy_frame/src/windows/pop_lock.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
 *
 */

#include "pop_lock.h"

LockPop::LockPop() :PopBase("@layout/pop_lock") {
    initUI();
}

LockPop::~LockPop() {
}

int8_t LockPop::getType() const {
    return POP_LOCK;
}

void LockPop::initUI() {
}