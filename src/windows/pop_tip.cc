/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:41:35
 * @FilePath: /hana_frame/src/windows/pop_tip.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */
#include "pop_tip.h"

TipPop::TipPop() :PopBase("@layout/pop_tip") {
    initUI();
}

TipPop::~TipPop() {
}

uint8_t TipPop::getType() const {
    return POP_TIP;
}

void TipPop::initUI() {
}
