/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:49:59
 * @LastEditTime: 2026-03-18 22:18:58
 * @FilePath: /kk_frame/src/app/page/components/wind_keyboard.h
 * @Description: 键盘组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_KEYBOARD_H__
#define __WIND_KEYBOARD_H__

#include <view/viewgroup.h>

#if defined(ENABLE_KEYBOARD)
#include "cKeyBoard.h"
#else
#include <widget/textview.h>
using CKeyBoard = TextView;
#endif

class WindKeyboard {
public:
    DECLARE_UIEVENT(void, OnCloseListener, const std::string &text);

private:
    CKeyBoard*            mKeyBoard;       // 键盘

    bool                  mIsInit;         // 是否初始化
    bool                  mIsShow;         // 是否显示

    OnCloseListener       mEnterListener;  // 回调函数
    OnCloseListener       mCancelListener; // 回调函数
public:
    WindKeyboard();
    ~WindKeyboard();

    void         init(ViewGroup* parent);
    void         onTick();
    virtual void showKeyboard(const std::string& text = "", const std::string& hint = "");
    virtual void hideKeyboard();
    bool         isKeyboardShow();
    bool         onKey(int keyCode, KeyEvent& evt, bool& result);

    void         setKeyboardMaxInputCount(int count);
    void         setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel);

private:
    inline bool  checkInit();
    void         onKeyBoardFinish(bool isEnter, const std::string& text);
};

#endif // !__WIND_KEYBOARD_H__
