/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-17 01:47:29
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.cc
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_en.h"

Keyboard_EN::Keyboard_EN(CKeyBoard* parent) :CKeyBoardChild(parent, "@layout/keyboard_en") {
}

CKeyBoard::KeyBoardType Keyboard_EN::getType() {
    return CKeyBoard::KB_TYPE_EN;
}
