/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-19 00:27:32
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_ru.cc
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_ru.h"

Keyboard_RU::Keyboard_RU(CKeyBoard* parent) :CKeyBoardChild(parent, "@keyboard:layout/keyboard_ru") {
    mKeyList.reserve(KEY_EN_MAX);

    mKeyStr[DISPLAY_TYPE_DEFAULT] = {
        "й","ц","у","к","е","н","г","ш","щ","з","х","ъ",
        "ф","ы","в","а","п","р","о","л","д","ж","э",
        "я","ч","с","м","и","т","ь","б","ю","ё",
        ",","пробел",".",
        "?123","#+=","RU","",""
    };
    mKeyStr[DISPLAY_TYPE_UPPER] = mKeyStr[DISPLAY_TYPE_UPPER_PLUS] = {
        "Й","Ц","У","К","Е","Н","Г","Ш","Щ","З","Х","Ъ",
        "Ф","Ы","В","А","П","Р","О","Л","Д","Ж","Э",
        "Я","Ч","С","М","И","Т","Ь","Б","Ю","Ё",
        ",","ПРОБЕЛ",".",
        "?123","#+=","RU","",""
    };
    mKeyStr[DISPLAY_TYPE_NUMBER] = {
        "1","2","3","4","5","6","7","8","9","0","@","#",
        "$","%","&","*","-","+","=","/","\\","_",":",
        "!","?","(",")","[","]","{","}","<",">",
        ".", "пробел", ",",
        "АБВ","#+=","RU","",""
    };
    mKeyStr[DISPLAY_TYPE_MORE] = {
        "«","»","—","–","…","№","§","±","×","÷","^","~",
        ":",";","`","₽","€","£","¥","₩","¢","©","®",
        "™","✓","•","°","‰","∞","∑","√","∆","π",
        ".", "пробел", "?",
        "АБВ","123","RU","",""
    };

    LOGI("Keyboard_RU::Keyboard_RU() Created");
}

CKeyBoard::KeyBoardType Keyboard_RU::getType() {
    return CKeyBoard::KB_TYPE_RU;
}

void Keyboard_RU::init() {
    getKeyList();
    setKeyAction();
}

void Keyboard_RU::onShow() {
    mRootView->setVisibility(View::VISIBLE);
    mDisplayType = DISPLAY_TYPE_DEFAULT;
    refreshDisplay();
}

void Keyboard_RU::onHide() {
    mRootView->setVisibility(View::GONE);
}

void Keyboard_RU::setKeyAction() {
    auto clickListener = [this](View& v) {
        onKeyClick(v.getId() - ID_START);
    };
    for (auto& btn : mKeyList) btn->setOnClickListener(clickListener);
}

void Keyboard_RU::onKeyClick(int key) {
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
        LOGE("Keyboard_RU::onKeyClick() Unknown key: %d", key);
    }   break;
    }

    // 检查最终类型
checkDisplayType:
    if (typeTag != mDisplayType) {
        mDisplayType = typeTag;
        refreshDisplay();
    }
}

void Keyboard_RU::refreshDisplay() {
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

void Keyboard_RU::getKeyList() {
#define PUSH_KEY_LIST(rid, key)                                        \
    do{                                                                \
        Button* btn = __dc(Button, mRootView->findViewById(rid));      \
        btn->setId(key + ID_START);                                    \
        mKeyList.push_back(btn);                                       \
    } while (0)

#define PUSH_KEY_LIST_BY_NUM(l, n) PUSH_KEY_LIST(LibRid::key_##l##_##n, KEY_EN_##l##_##n)

    // 插入顺序必须与枚举 KEY_EN_XXX 一致，提高性能
    PUSH_KEY_LIST_BY_NUM(1, 1);
    PUSH_KEY_LIST_BY_NUM(1, 2);
    PUSH_KEY_LIST_BY_NUM(1, 3);
    PUSH_KEY_LIST_BY_NUM(1, 4);
    PUSH_KEY_LIST_BY_NUM(1, 5);
    PUSH_KEY_LIST_BY_NUM(1, 6);
    PUSH_KEY_LIST_BY_NUM(1, 7);
    PUSH_KEY_LIST_BY_NUM(1, 8);
    PUSH_KEY_LIST_BY_NUM(1, 9);
    PUSH_KEY_LIST_BY_NUM(1, 10);
    PUSH_KEY_LIST_BY_NUM(1, 11);
    PUSH_KEY_LIST_BY_NUM(1, 12);

    PUSH_KEY_LIST_BY_NUM(2, 1);
    PUSH_KEY_LIST_BY_NUM(2, 2);
    PUSH_KEY_LIST_BY_NUM(2, 3);
    PUSH_KEY_LIST_BY_NUM(2, 4);
    PUSH_KEY_LIST_BY_NUM(2, 5);
    PUSH_KEY_LIST_BY_NUM(2, 6);
    PUSH_KEY_LIST_BY_NUM(2, 7);
    PUSH_KEY_LIST_BY_NUM(2, 8);
    PUSH_KEY_LIST_BY_NUM(2, 9);
    PUSH_KEY_LIST_BY_NUM(2, 10);
    PUSH_KEY_LIST_BY_NUM(2, 11);

    PUSH_KEY_LIST_BY_NUM(3, 1);
    PUSH_KEY_LIST_BY_NUM(3, 2);
    PUSH_KEY_LIST_BY_NUM(3, 3);
    PUSH_KEY_LIST_BY_NUM(3, 4);
    PUSH_KEY_LIST_BY_NUM(3, 5);
    PUSH_KEY_LIST_BY_NUM(3, 6);
    PUSH_KEY_LIST_BY_NUM(3, 7);
    PUSH_KEY_LIST_BY_NUM(3, 8);
    PUSH_KEY_LIST_BY_NUM(3, 9);
    PUSH_KEY_LIST_BY_NUM(3, 10);

    PUSH_KEY_LIST(LibRid::key_left, KEY_EN_LEFT);
    PUSH_KEY_LIST(LibRid::key_space, KEY_EN_SPACE);
    PUSH_KEY_LIST(LibRid::key_right, KEY_EN_RIGHT);

    PUSH_KEY_LIST(LibRid::key_math, KEY_EN_MATH);
    PUSH_KEY_LIST(LibRid::key_more, KEY_EN_MORE);
    PUSH_KEY_LIST(LibRid::key_lang, KEY_EN_LANG);
    PUSH_KEY_LIST(LibRid::key_shift, KEY_EN_SHIFT);
    PUSH_KEY_LIST(LibRid::key_backspace, KEY_EN_BACKSPACE);

#undef PUSH_KEY_LIST_BY_NUM
#undef PUSH_KEY_LIST
}

void Keyboard_RU::setKeyStr() {
    auto nowTypeStr = mKeyStr.at(mDisplayType);
    for (int i = 0; i < mKeyList.size(); i++)
        mKeyList[i]->setText(nowTypeStr[i]);
}
