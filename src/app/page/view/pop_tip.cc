/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-02-13 16:26:20
 * @LastEditTime: 2026-08-11 17:57:37
 * @FilePath: /kk_frame/src/app/page/view/pop_tip.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "pop_tip.h"

POP_REGISTER(POP_TIP, TipPop);

TipPop::TipPop() :PopBase("@layout/pop_tip") {
    setGauss();
    initUI();
}

TipPop::~TipPop() { }

int8_t TipPop::getType() const {
    return POP_TIP;
}

void TipPop::initUI() {
    click(AppRid::btn_ok, [this](View&) {close();});
}
