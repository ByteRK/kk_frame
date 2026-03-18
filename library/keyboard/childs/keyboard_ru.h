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

#include "keyboard_base.h"
#include <array>

/// @brief 俄语键盘
class Keyboard_RU : public CKeyBoardChild {
protected:
    static constexpr int ID_START = 20000;
    enum {
        KEY_EN_1_1, KEY_EN_1_2, KEY_EN_1_3, KEY_EN_1_4, KEY_EN_1_5, KEY_EN_1_6, KEY_EN_1_7, KEY_EN_1_8, KEY_EN_1_9, KEY_EN_1_10, KEY_EN_1_11, KEY_EN_1_12,
        KEY_EN_2_1, KEY_EN_2_2, KEY_EN_2_3, KEY_EN_2_4, KEY_EN_2_5, KEY_EN_2_6, KEY_EN_2_7, KEY_EN_2_8, KEY_EN_2_9, KEY_EN_2_10, KEY_EN_2_11,
        KEY_EN_3_1, KEY_EN_3_2, KEY_EN_3_3, KEY_EN_3_4, KEY_EN_3_5, KEY_EN_3_6, KEY_EN_3_7, KEY_EN_3_8, KEY_EN_3_9, KEY_EN_3_10,
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
    Keyboard_RU(CKeyBoard* parent);

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

#endif // __KEYBOARD_RU_H__