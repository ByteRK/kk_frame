/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-02-13 16:26:20
 * @LastEditTime: 2025-12-29 14:23:50
 * @FilePath: /kk_frame/src/app/page/view/pop_lock.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "pop_lock.h"

POP_REGISTER(POP_LOCK, LockPop);

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