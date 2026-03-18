/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-17 18:35:22
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.cc
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_en.h"

Keyboard_EN::Keyboard_EN(CKeyBoard* parent, const std::string& layout) :CKeyBoardChild(parent, layout) {
    mKeyList.reserve(KEY_EN_MAX);

    mKeyStr[DISPLAY_TYPE_DEFAULT] = {
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
        "a", "s", "d", "f", "g", "h", "j", "k", "l",
        "z", "x", "c", "v", "b", "n", "m",
        ",", "空格", ".",
        "?123", "更多", "英文", "", ""
    };
    mKeyStr[DISPLAY_TYPE_UPPER_PLUS] = mKeyStr[DISPLAY_TYPE_UPPER] = {
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
        "A", "S", "D", "F", "G", "H", "J", "K", "L",
        "Z", "X", "C", "V", "B", "N", "M",
        ",", "空格", ".",
        "?123", "更多", "英文", "", ""
    };
    mKeyStr[DISPLAY_TYPE_NUMBER] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "+", "-", "*", "/", "=", "·", "&", "(", ")",
        "~", ",", "...", "@", "!", "'", "\"",
        ".", "空格", "?",
        "返回", "更多", "英文", "", ""
    };
    mKeyStr[DISPLAY_TYPE_MORE] = {
        "[", "]", "{", "}", "#", "%", "^", ";", ":", "_",
        "ˇ", "|", "\\", "<", ">", "￥", "€", "£", "₤",
        "~", ",", "$", "@", "!", "`", "\"",
        ".", "空格", "?",
        "返回", "123", "英文", "", ""
    };

    LOGI("Keyboard_EN::Keyboard_EN() Created");
}

CKeyBoard::KeyBoardType Keyboard_EN::getType() {
    return CKeyBoard::KB_TYPE_EN;
}

void Keyboard_EN::init() {
    getKeyList();
    setKeyAction();
}

void Keyboard_EN::onShow() {
    mRootView->setVisibility(View::VISIBLE);
    mDisplayType = DISPLAY_TYPE_DEFAULT;
    refreshDisplay();
}

void Keyboard_EN::onHide() {
    mRootView->setVisibility(View::GONE);
}

void Keyboard_EN::setKeyAction() {
    auto clickListener = [this](View& v) {
        onKeyClick(v.getId() - ID_START);
    };
    for (auto& btn : mKeyList) btn->setOnClickListener(clickListener);
}

void Keyboard_EN::onKeyClick(int key) {
    // Shift键逻辑特殊处理
    DISPLAY_TYPE typeTag = mDisplayType;
    if (key != KEY_EN_SHIFT) {
        mShiftLastClickTime = 0;
        if (mDisplayType == DISPLAY_TYPE_UPPER)
            typeTag = DISPLAY_TYPE_DEFAULT;
    }

    // 常规按键
    if (key <= KEY_EN_RIGHT) {
        mParent->appendText(
            key == KEY_EN_SPACE ? " " :
            mKeyStr[mDisplayType][key]
        );
        goto checkDisplayType;
    }

    // 特殊操作按键
    switch (key) {
    case KEY_EN_MATH: {
        if (mDisplayType == DISPLAY_TYPE_NUMBER || mDisplayType == DISPLAY_TYPE_MORE)
            typeTag = DISPLAY_TYPE_DEFAULT;
        else
            typeTag = DISPLAY_TYPE_NUMBER;
    }   break;
    case KEY_EN_MORE: {
        if (mDisplayType == DISPLAY_TYPE_NUMBER)
            typeTag = DISPLAY_TYPE_MORE;
        else
            typeTag = DISPLAY_TYPE_NUMBER;
    }   break;
    case KEY_EN_SHIFT: {
        int64_t now = SystemClock::uptimeMillis();
        if (mDisplayType == DISPLAY_TYPE_UPPER) {
            if (now - mShiftLastClickTime < 500)
                typeTag = DISPLAY_TYPE_UPPER_PLUS;
            else
                typeTag = DISPLAY_TYPE_DEFAULT;
        } else if (mDisplayType == DISPLAY_TYPE_UPPER_PLUS) {
            typeTag = DISPLAY_TYPE_DEFAULT;
        } else {
            typeTag = DISPLAY_TYPE_UPPER;
        }
        mShiftLastClickTime = now;
    }   break;
    case KEY_EN_BACKSPACE: {
        mParent->backspaceText();
    }   break;
    case KEY_EN_LANG: {
        mParent->showNextType();
    }   break;
    default: {
        LOGE("Keyboard_EN::onKeyClick() Unknown key: %d", key);
    }   break;
    }

    // 检查最终类型
checkDisplayType:
    if (typeTag != mDisplayType) {
        mDisplayType = typeTag;
        refreshDisplay();
    }
}

void Keyboard_EN::refreshDisplay() {
    setKeyStr();

    mKeyList[KEY_EN_SHIFT]->setActivated(false);
    mKeyList[KEY_EN_SHIFT]->setSelected(false);
    mKeyList[KEY_EN_SHIFT]->setVisibility(View::VISIBLE);
    mKeyList[KEY_EN_MORE]->setVisibility(View::GONE);

    switch (mDisplayType) {
    case DISPLAY_TYPE_UPPER: {
        mKeyList[KEY_EN_SHIFT]->setActivated(true);
    }   break;
    case DISPLAY_TYPE_UPPER_PLUS: {
        mKeyList[KEY_EN_SHIFT]->setSelected(true);
    }   break;
    case DISPLAY_TYPE_NUMBER: {
        mKeyList[KEY_EN_SHIFT]->setVisibility(View::GONE);
        mKeyList[KEY_EN_MORE]->setVisibility(View::VISIBLE);
    }   break;
    case DISPLAY_TYPE_MORE: {
        mKeyList[KEY_EN_SHIFT]->setVisibility(View::GONE);
        mKeyList[KEY_EN_MORE]->setVisibility(View::VISIBLE);
    }   break;
    default: {
    }   break;
    }
}

void Keyboard_EN::getKeyList() {
#define PUSH_KEY_LIST(rid, key)                                        \
    do{                                                                \
        Button* btn = __dc(Button, mRootView->findViewById(rid));      \
        btn->setId(key + ID_START);                                    \
        mKeyList.push_back(btn);                                       \
    } while (0)

    // 插入顺序必须与枚举 KEY_EN_XXX 一致，提高性能
    PUSH_KEY_LIST(LibRid::key_q, KEY_EN_Q);
    PUSH_KEY_LIST(LibRid::key_w, KEY_EN_W);
    PUSH_KEY_LIST(LibRid::key_e, KEY_EN_E);
    PUSH_KEY_LIST(LibRid::key_r, KEY_EN_R);
    PUSH_KEY_LIST(LibRid::key_t, KEY_EN_T);
    PUSH_KEY_LIST(LibRid::key_y, KEY_EN_Y);
    PUSH_KEY_LIST(LibRid::key_u, KEY_EN_U);
    PUSH_KEY_LIST(LibRid::key_i, KEY_EN_I);
    PUSH_KEY_LIST(LibRid::key_o, KEY_EN_O);
    PUSH_KEY_LIST(LibRid::key_p, KEY_EN_P);
    PUSH_KEY_LIST(LibRid::key_a, KEY_EN_A);
    PUSH_KEY_LIST(LibRid::key_s, KEY_EN_S);
    PUSH_KEY_LIST(LibRid::key_d, KEY_EN_D);
    PUSH_KEY_LIST(LibRid::key_f, KEY_EN_F);
    PUSH_KEY_LIST(LibRid::key_g, KEY_EN_G);
    PUSH_KEY_LIST(LibRid::key_h, KEY_EN_H);
    PUSH_KEY_LIST(LibRid::key_j, KEY_EN_J);
    PUSH_KEY_LIST(LibRid::key_k, KEY_EN_K);
    PUSH_KEY_LIST(LibRid::key_l, KEY_EN_L);
    PUSH_KEY_LIST(LibRid::key_z, KEY_EN_Z);
    PUSH_KEY_LIST(LibRid::key_x, KEY_EN_X);
    PUSH_KEY_LIST(LibRid::key_c, KEY_EN_C);
    PUSH_KEY_LIST(LibRid::key_v, KEY_EN_V);
    PUSH_KEY_LIST(LibRid::key_b, KEY_EN_B);
    PUSH_KEY_LIST(LibRid::key_n, KEY_EN_N);
    PUSH_KEY_LIST(LibRid::key_m, KEY_EN_M);

    PUSH_KEY_LIST(LibRid::key_left, KEY_EN_LEFT);
    PUSH_KEY_LIST(LibRid::key_space, KEY_EN_SPACE);
    PUSH_KEY_LIST(LibRid::key_right, KEY_EN_RIGHT);

    PUSH_KEY_LIST(LibRid::key_math, KEY_EN_MATH);
    PUSH_KEY_LIST(LibRid::key_more, KEY_EN_MORE);
    PUSH_KEY_LIST(LibRid::key_lang, KEY_EN_LANG);
    PUSH_KEY_LIST(LibRid::key_shift, KEY_EN_SHIFT);
    PUSH_KEY_LIST(LibRid::key_backspace, KEY_EN_BACKSPACE);

#undef PUSH_KEY_LIST
}

void Keyboard_EN::setKeyStr() {
    auto nowTypeStr = mKeyStr.at(mDisplayType);
    for (int i = 0; i < mKeyList.size(); i++)
        mKeyList[i]->setText(nowTypeStr[i]);
}
