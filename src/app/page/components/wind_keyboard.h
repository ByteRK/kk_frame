/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:49:59
 * @LastEditTime: 2026-09-24 11:57:40
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
/// @brief 键盘默认最大输入长度（跟随键盘库默认值）
static constexpr int KEYBOARD_DEFAULT_INPUT_LIMIT = CKeyBoard::DEFAULT_INPUT_LIMIT;
#else
#include <widget/textview.h>
using CKeyBoard = TextView;
static constexpr int KEYBOARD_DEFAULT_INPUT_LIMIT = 20;
#endif

class WindKeyboard {
public:
    DECLARE_UIEVENT(void, OnCloseListener, const std::string &text);
    DECLARE_UIEVENT(void, OnMaxLengthListener, int maxCount);

private:
    CKeyBoard*            mKeyBoard{ nullptr };       // 键盘
    ViewGroup*            mKeyBoardRoot{ nullptr };   // 键盘根布局

    bool                  mIsInit{ false };           // 是否初始化
    bool                  mIsShow{ false };           // 是否显示
    int                   mMaxInputCount{ KEYBOARD_DEFAULT_INPUT_LIMIT };  // 最大输入长度（<=0 不限制）

    bool                  mGaussEnable{ true };        // 是否启用背景模糊
    int                   mGaussRadius{ 10 };          // 背景模糊半径
    uint64_t              mGaussColor{ 0xaa000000 };   // 背景模糊蒙版颜色

    OnCloseListener       mEnterListener{ nullptr };     // 回调函数
    OnCloseListener       mCancelListener{ nullptr };    // 回调函数
    OnMaxLengthListener   mMaxLengthListener{ nullptr }; // 输入长度达到上限回调
public:
    WindKeyboard();
    virtual ~WindKeyboard();

    virtual void showKeyboard(const std::string& text = "", const std::string& hint = "");
    virtual void hideKeyboard();
    bool         isKeyboardShow() const;

    void         setKeyboardMaxInputCount(int count);
    void         setKeyboardEditChangeCallBack(OnCloseListener listener);
    void         setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel);
    void         setKeyboardMaxLengthCallBack(OnMaxLengthListener listener);
    void         setKeyboardGauss(bool enable = true, int radius = 10, uint64_t color = 0xaa000000);

protected:
    void         init(ViewGroup* parent);
    bool         onKey(int keyCode, KeyEvent& evt, bool& result);

private:
    bool         checkInit();
    void         clearCallbacks();
    void         onKeyBoardFinish(bool isEnter, const std::string& text);
    void         applyGauss();
};

#endif // !__WIND_KEYBOARD_H__
