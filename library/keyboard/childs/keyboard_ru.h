/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-18 23:56:55
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_ru.h
 * @Description: 俄语键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_RU_H__
#define __KEYBOARD_RU_H__

#include "keyboard_qwerty.h"

/// @brief 俄语键盘
class Keyboard_RU : public Keyboard_Qwerty {
protected:
    /// @note 枚举值即 mKeyList 下标，必须与按键表、collectKeys() 顺序保持一致
    enum {
        KEY_RU_1_1, KEY_RU_1_2, KEY_RU_1_3, KEY_RU_1_4, KEY_RU_1_5, KEY_RU_1_6, KEY_RU_1_7, KEY_RU_1_8, KEY_RU_1_9, KEY_RU_1_10, KEY_RU_1_11, KEY_RU_1_12,
        KEY_RU_2_1, KEY_RU_2_2, KEY_RU_2_3, KEY_RU_2_4, KEY_RU_2_5, KEY_RU_2_6, KEY_RU_2_7, KEY_RU_2_8, KEY_RU_2_9, KEY_RU_2_10, KEY_RU_2_11,
        KEY_RU_3_1, KEY_RU_3_2, KEY_RU_3_3, KEY_RU_3_4, KEY_RU_3_5, KEY_RU_3_6, KEY_RU_3_7, KEY_RU_3_8, KEY_RU_3_9, KEY_RU_3_10,
        KEY_RU_LEFT, KEY_RU_SPACE, KEY_RU_RIGHT,

        KEY_RU_MATH, KEY_RU_MORE, KEY_RU_LANG, KEY_RU_SHIFT, KEY_RU_BACKSPACE,

        KEY_RU_MAX
    };
    /// @brief 字母区按键数量（其后为 左/空格/右 等尾部按键）
    static constexpr int MAIN_KEY_COUNT = KEY_RU_LEFT;

public:
    Keyboard_RU(CKeyBoard* parent);

protected:
    virtual CKeyBoard::KeyBoardType getType() override;
    virtual void onShow() override;

protected:
    virtual void collectKeys() override;
};

#endif // __KEYBOARD_RU_H__