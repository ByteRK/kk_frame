/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-18 09:49:19
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.h
 * @Description: 英文键盘 可作为一个基本键盘进行派生
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_EN_H__
#define __KEYBOARD_EN_H__

#include "keyboard_base.h"
#include <array>

/// @brief 英文键盘
class Keyboard_EN : public CKeyBoardChild {
protected:
    static constexpr int ID_START = 20000;
    enum {
        KEY_EN_Q, KEY_EN_W, KEY_EN_E, KEY_EN_R, KEY_EN_T, KEY_EN_Y, KEY_EN_U, KEY_EN_I, KEY_EN_O, KEY_EN_P,
        KEY_EN_A, KEY_EN_S, KEY_EN_D, KEY_EN_F, KEY_EN_G, KEY_EN_H, KEY_EN_J, KEY_EN_K, KEY_EN_L,
        KEY_EN_Z, KEY_EN_X, KEY_EN_C, KEY_EN_V, KEY_EN_B, KEY_EN_N, KEY_EN_M,
        KEY_EN_LEFT, KEY_EN_SPACE, KEY_EN_RIGHT,

        KEY_EN_MATH, KEY_EN_MORE, KEY_EN_LANG, KEY_EN_SHIFT, KEY_EN_BACKSPACE,

        KEY_EN_MAX
    };
    typedef enum {
        DISPLAY_TYPE_DEFAULT = 0,   // 常规
        DISPLAY_TYPE_UPPER,         // 大写
        DISPLAY_TYPE_UPPER_PLUS,    // 大写锁定
        DISPLAY_TYPE_NUMBER,        // 数字
        DISPLAY_TYPE_MORE,          // 更多

        DISPLAY_TYPE_MAX
    } DISPLAY_TYPE;
    using KeyTextType = const char*;

protected:
    DISPLAY_TYPE         mDisplayType{ DISPLAY_TYPE_DEFAULT };                   // 显示模式
    std::vector<Button*>                                              mKeyList;  // 按键列表
    std::array<std::array<KeyTextType, KEY_EN_MAX>, DISPLAY_TYPE_MAX> mKeyStr;   // 按键字符

private:
    int64_t mShiftLastClickTime{ 0 };  // 上次shift按键点击时间

public:
    Keyboard_EN(CKeyBoard* parent, const std::string& layout = "@keyboard:layout/keyboard_en");

protected:
    virtual CKeyBoard::KeyBoardType getType() override;
    virtual void init() override;
    virtual void onShow() override;
    virtual void onHide() override;

protected:
    void setKeyAction();
    virtual void onKeyClick(int key);
    virtual void refreshDisplay();

private:
    void getKeyList();
    void setKeyStr();
};

#endif // __KEYBOARD_EN_H__