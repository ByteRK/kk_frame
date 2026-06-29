/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:49:59
 * @LastEditTime: 2026-06-30 00:58:01
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
#include "quick_define.h"

#if ENABLED(KEYBOARD)
#include "cKeyBoard.h"
#else
#include <widget/textview.h>
using CKeyBoard = TextView;
#endif

class WindKeyboard {
public:
    DECLARE_UIEVENT(void, OnCloseListener, const std::string &text);

private:
    CKeyBoard*            mKeyBoard{ nullptr };       // 键盘

    bool                  mIsInit{ false };           // 是否初始化
    bool                  mIsShow{ false };           // 是否显示

    OnCloseListener       mEnterListener{ nullptr };  // 回调函数
    OnCloseListener       mCancelListener{ nullptr }; // 回调函数
public:
    WindKeyboard();
    virtual ~WindKeyboard();

    virtual void showKeyboard(const std::string& text = "", const std::string& hint = "");
    virtual void hideKeyboard();
    bool         isKeyboardShow() const;

    void         setKeyboardMaxInputCount(int count);
    void         setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel);

protected:
    void         init(ViewGroup* parent);
    bool         onKey(int keyCode, KeyEvent& evt, bool& result);

private:
    bool         checkInit();
    void         onKeyBoardFinish(bool isEnter, const std::string& text);
};

#endif // !__WIND_KEYBOARD_H__
