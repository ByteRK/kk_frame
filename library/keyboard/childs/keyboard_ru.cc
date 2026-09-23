/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-09-23 10:26:29
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_ru.cc
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_ru.h"

Keyboard_RU::Keyboard_RU(CKeyBoard* parent) :Keyboard_Qwerty(parent, "@keyboard:layout/keyboard_ru", MAIN_KEY_COUNT) {
    setKeyStr(DISPLAY_TYPE_DEFAULT, {
        "й","ц","у","к","е","н","г","ш","щ","з","х","ъ",
        "ф","ы","в","а","п","р","о","л","д","ж","э",
        "я","ч","с","м","и","т","ь","б","ю","ё",
        ",","пробел",".",
        "?123","#+=","RU","",""
        });
    setKeyStr(DISPLAY_TYPE_UPPER, {
        "Й","Ц","У","К","Е","Н","Г","Ш","Щ","З","Х","Ъ",
        "Ф","Ы","В","А","П","Р","О","Л","Д","Ж","Э",
        "Я","Ч","С","М","И","Т","Ь","Б","Ю","Ё",
        ",","ПРОБЕЛ",".",
        "?123","#+=","RU","",""
        });
    copyKeyStr(DISPLAY_TYPE_UPPER, DISPLAY_TYPE_UPPER_PLUS);
    setKeyStr(DISPLAY_TYPE_NUMBER, {
        "1","2","3","4","5","6","7","8","9","0","@","#",
        "$","%","&","*","-","+","=","/","\\","_",":",
        "!","?","(",")","[","]","{","}","<",">",
        ".", "пробел", ",",
        "АБВ","#+=","RU","",""
        });
    setKeyStr(DISPLAY_TYPE_MORE, {
        "«","»","—","–","…","№","§","±","×","÷","^","~",
        ":",";","`","₽","€","£","¥","₩","¢","©","®",
        "™","✓","•","°","‰","∞","∑","√","∆","π",
        ".", "пробел", "?",
        "АБВ","123","RU","",""
        });

    LOGI("Keyboard_RU::Keyboard_RU() Created");
}

CKeyBoard::KeyBoardType Keyboard_RU::getType() {
    return CKeyBoard::KB_TYPE_RU;
}

void Keyboard_RU::onShow() {
    Keyboard_Qwerty::onShow();
    updateParentBtn("ОК", "Отмена");
}

void Keyboard_RU::collectKeys() {
#define PUSH_KEY_LIST(rid)                                             \
    do{                                                                \
        Button* btn = __dc(Button, mRootView->findViewById(rid));      \
        FailFast(!btn, "keyboard key not found: id=0x%x", rid);        \
        mKeyList.push_back(btn);                                       \
    } while (0)

#define PUSH_KEY_LIST_BY_NUM(l, n) PUSH_KEY_LIST(LibRid::key_##l##_##n)

    // 插入顺序必须与枚举 KEY_RU_XXX 一致，提高性能
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

    PUSH_KEY_LIST(LibRid::key_left);
    PUSH_KEY_LIST(LibRid::key_space);
    PUSH_KEY_LIST(LibRid::key_right);

    PUSH_KEY_LIST(LibRid::key_math);
    PUSH_KEY_LIST(LibRid::key_more);
    PUSH_KEY_LIST(LibRid::key_lang);
    PUSH_KEY_LIST(LibRid::key_shift);
    PUSH_KEY_LIST(LibRid::key_backspace);

#undef PUSH_KEY_LIST_BY_NUM
#undef PUSH_KEY_LIST
}
