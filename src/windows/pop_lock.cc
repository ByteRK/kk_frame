/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:12:59
 * @FilePath: /hana_frame/src/windows/pop_lock.cc
 * @Description: 童锁弹窗
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#include "pop_lock.h"

LockPop::LockPop() :PopBase("@layout/pop_lock") {
    initUI();
}

LockPop::~LockPop() {
}

uint8_t LockPop::getType() const {
    return POP_LOCK;
}

void LockPop::initUI() {
}