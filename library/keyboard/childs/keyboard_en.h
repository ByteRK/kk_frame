/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-17 01:30:17
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.h
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_EN_H__
#define __KEYBOARD_EN_H__

#include "keyboard_base.h"

class Keyboard_EN : public CKeyBoardChild {
public:
    Keyboard_EN(CKeyBoard* parent);
    CKeyBoard::KeyBoardType getType() override;
};

#endif // __KEYBOARD_EN_H__