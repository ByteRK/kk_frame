/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-18 22:35:18
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.h
 * @Description: 英文键盘 可作为一个基本键盘进行派生
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_EN_H__
#define __KEYBOARD_EN_H__

#include "keyboard_qwerty.h"

/// @brief 英文键盘
class Keyboard_EN : public Keyboard_Qwerty {
protected:
    /// @note 枚举值即 mKeyList 下标，必须与按键表、collectKeys() 顺序保持一致
    enum {
        KEY_EN_Q, KEY_EN_W, KEY_EN_E, KEY_EN_R, KEY_EN_T, KEY_EN_Y, KEY_EN_U, KEY_EN_I, KEY_EN_O, KEY_EN_P,
        KEY_EN_A, KEY_EN_S, KEY_EN_D, KEY_EN_F, KEY_EN_G, KEY_EN_H, KEY_EN_J, KEY_EN_K, KEY_EN_L,
        KEY_EN_Z, KEY_EN_X, KEY_EN_C, KEY_EN_V, KEY_EN_B, KEY_EN_N, KEY_EN_M,
        KEY_EN_LEFT, KEY_EN_SPACE, KEY_EN_RIGHT,

        KEY_EN_MATH, KEY_EN_MORE, KEY_EN_LANG, KEY_EN_SHIFT, KEY_EN_BACKSPACE,

        KEY_EN_MAX
    };
    /// @brief 字母区按键数量（其后为 左/空格/右 等尾部按键）
    static constexpr int MAIN_KEY_COUNT = KEY_EN_LEFT;

public:
    Keyboard_EN(CKeyBoard* parent, const std::string& layout = "@keyboard:layout/keyboard_en");

protected:
    virtual CKeyBoard::KeyBoardType getType() override;
    virtual void onShow() override;
    virtual void onRealKey(int key) override;

protected:
    virtual void collectKeys() override;
};

#endif // __KEYBOARD_EN_H__