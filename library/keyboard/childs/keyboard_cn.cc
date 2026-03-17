/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-03-17 18:44:09
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_cn.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_cn.h"

Keyboard_CN::Keyboard_CN(CKeyBoard* parent) :Keyboard_EN(parent, "@layout/keyboard_cn") {
    for (uint8_t i = 0; i < DISPLAY_TYPE_MAX; i++) {
        mKeyStr[i][KEY_EN_LANG] = "拼音";
        mKeyStr[i][KEY_EN_LEFT] = "，";
        mKeyStr[i][KEY_EN_RIGHT] = "。";

        // TODO
    }

    LOGI("Keyboard_CN::Keyboard_CN() Created");
}

CKeyBoard::KeyBoardType Keyboard_CN::getType() {
    return CKeyBoard::KB_TYPE_CN;
}

void Keyboard_CN::init() {
    Keyboard_EN::init();
    mCandidateBoxes = __dc(ViewGroup, mRootView->findViewById(AppRid::candidate_box));
    mCandidateBoxes->setEnabled(false);
}
