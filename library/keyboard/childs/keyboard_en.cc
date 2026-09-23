/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:05:31
 * @LastEditTime: 2026-03-18 23:24:44
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_en.cc
 * @Description: 英文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_en.h"

Keyboard_EN::Keyboard_EN(CKeyBoard* parent, const std::string& layout)
    : Keyboard_Qwerty(parent, layout, MAIN_KEY_COUNT) {
    setKeyStr(DISPLAY_TYPE_DEFAULT, {
        "q","w","e","r","t","y","u","i","o","p",
        "a","s","d","f","g","h","j","k","l",
        "z","x","c","v","b","n","m",
        ",","Space",".",
        "?123","#+=","ENG","",""
        });
    setKeyStr(DISPLAY_TYPE_UPPER, {
        "Q","W","E","R","T","Y","U","I","O","P",
        "A","S","D","F","G","H","J","K","L",
        "Z","X","C","V","B","N","M",
        ",","Space",".",
        "?123","#+=","ENG","",""
        });
    copyKeyStr(DISPLAY_TYPE_UPPER, DISPLAY_TYPE_UPPER_PLUS);
    setKeyStr(DISPLAY_TYPE_NUMBER, {
        "1","2","3","4","5","6","7","8","9","0",
        "@","#","$","%","&","*","-","+","=",
        "!","?","(",")","_","/","\"",
        ".", "Space", ",",
        "ABC","#+=","ENG","",""
        });
    setKeyStr(DISPLAY_TYPE_MORE, {
        "[","]","{","}","<",">","^","|","\\","~",
        ":",";","`","€","£","¥","₩","¢","§",
        "...","·","(",")","_","/","\"",
        ".", "Space", "?",
        "ABC","123","ENG","",""
        });

    LOGI("Keyboard_EN::Keyboard_EN() Created");
}

CKeyBoard::KeyBoardType Keyboard_EN::getType() {
    return CKeyBoard::KB_TYPE_EN;
}

void Keyboard_EN::onShow() {
    Keyboard_Qwerty::onShow();
    updateParentBtn("OK", "Cancel");
}

void Keyboard_EN::onRealKey(int key) {
#define REAL_KEY_ACTION(k) case KeyEvent::KEYCODE_##k: { mKeyList[KEY_EN_##k]->performClick(); } break
    switch (key) {
        REAL_KEY_ACTION(Q);
        REAL_KEY_ACTION(W);
        REAL_KEY_ACTION(E);
        REAL_KEY_ACTION(R);
        REAL_KEY_ACTION(T);
        REAL_KEY_ACTION(Y);
        REAL_KEY_ACTION(U);
        REAL_KEY_ACTION(I);
        REAL_KEY_ACTION(O);
        REAL_KEY_ACTION(P);
        REAL_KEY_ACTION(A);
        REAL_KEY_ACTION(S);
        REAL_KEY_ACTION(D);
        REAL_KEY_ACTION(F);
        REAL_KEY_ACTION(G);
        REAL_KEY_ACTION(H);
        REAL_KEY_ACTION(J);
        REAL_KEY_ACTION(K);
        REAL_KEY_ACTION(L);
        REAL_KEY_ACTION(Z);
        REAL_KEY_ACTION(X);
        REAL_KEY_ACTION(C);
        REAL_KEY_ACTION(V);
        REAL_KEY_ACTION(B);
        REAL_KEY_ACTION(N);
        REAL_KEY_ACTION(M);
    case KeyEvent::KEYCODE_SHIFT_LEFT:
    case KeyEvent::KEYCODE_SHIFT_RIGHT: {
        mKeyList[KEY_EN_LANG]->performClick();
    }   break;
    default: {
        LOGW("Keyboard_EN::onRealKey() Unknown key: %d", key);
    }   break;
    }
#undef REAL_KEY_ACTION
}

void Keyboard_EN::collectKeys() {
#define PUSH_KEY_LIST(rid)                                             \
    do{                                                                \
        Button* btn = __dc(Button, mRootView->findViewById(rid));      \
        FailFast(!btn, "keyboard key not found: id=0x%x", rid);        \
        mKeyList.push_back(btn);                                       \
    } while (0)

    // 插入顺序必须与枚举 KEY_EN_XXX 一致，提高性能
    PUSH_KEY_LIST(LibRid::key_q);
    PUSH_KEY_LIST(LibRid::key_w);
    PUSH_KEY_LIST(LibRid::key_e);
    PUSH_KEY_LIST(LibRid::key_r);
    PUSH_KEY_LIST(LibRid::key_t);
    PUSH_KEY_LIST(LibRid::key_y);
    PUSH_KEY_LIST(LibRid::key_u);
    PUSH_KEY_LIST(LibRid::key_i);
    PUSH_KEY_LIST(LibRid::key_o);
    PUSH_KEY_LIST(LibRid::key_p);
    PUSH_KEY_LIST(LibRid::key_a);
    PUSH_KEY_LIST(LibRid::key_s);
    PUSH_KEY_LIST(LibRid::key_d);
    PUSH_KEY_LIST(LibRid::key_f);
    PUSH_KEY_LIST(LibRid::key_g);
    PUSH_KEY_LIST(LibRid::key_h);
    PUSH_KEY_LIST(LibRid::key_j);
    PUSH_KEY_LIST(LibRid::key_k);
    PUSH_KEY_LIST(LibRid::key_l);
    PUSH_KEY_LIST(LibRid::key_z);
    PUSH_KEY_LIST(LibRid::key_x);
    PUSH_KEY_LIST(LibRid::key_c);
    PUSH_KEY_LIST(LibRid::key_v);
    PUSH_KEY_LIST(LibRid::key_b);
    PUSH_KEY_LIST(LibRid::key_n);
    PUSH_KEY_LIST(LibRid::key_m);

    PUSH_KEY_LIST(LibRid::key_left);
    PUSH_KEY_LIST(LibRid::key_space);
    PUSH_KEY_LIST(LibRid::key_right);

    PUSH_KEY_LIST(LibRid::key_math);
    PUSH_KEY_LIST(LibRid::key_more);
    PUSH_KEY_LIST(LibRid::key_lang);
    PUSH_KEY_LIST(LibRid::key_shift);
    PUSH_KEY_LIST(LibRid::key_backspace);

#undef PUSH_KEY_LIST
}
