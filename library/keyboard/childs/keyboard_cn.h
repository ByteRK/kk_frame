/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-03-17 18:40:43
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_cn.h
 * @Description: 中文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_CN_H__
#define __KEYBOARD_CN_H__

#include "keyboard_en.h"

#include "view/viewgroup.h"

/// @brief 中文键盘
class Keyboard_CN : public Keyboard_EN {
private:
    ViewGroup* mCandidateBoxes{ nullptr }; // 候选框

public:
    Keyboard_CN(CKeyBoard* parent);

protected:
    CKeyBoard::KeyBoardType getType() override;
    void init() override;
};

#endif // __KEYBOARD_CN_H__