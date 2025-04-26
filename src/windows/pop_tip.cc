/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-02-13 16:26:20
 * @LastEditTime: 2025-02-18 19:54:57
 * @FilePath: /cy_frame/src/windows/pop_tip.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
 *
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
