/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:49:59
 * @LastEditTime: 2026-08-10 10:03:06
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
    /// @brief 输入长度达到上限回调（参数为上限值）
    DECLARE_UIEVENT(void, OnMaxLengthListener, int maxCount);

private:
    CKeyBoard*            mKeyBoard{ nullptr };       // 键盘

    bool                  mIsInit{ false };           // 是否初始化
    bool                  mIsShow{ false };           // 是否显示
    int                   mMaxInputCount{ KEYBOARD_DEFAULT_INPUT_LIMIT };  // 最大输入长度（<=0 不限制）

    OnCloseListener       mEnterListener{ nullptr };     // 回调函数
    OnCloseListener       mCancelListener{ nullptr };    // 回调函数
    OnMaxLengthListener   mMaxLengthListener{ nullptr }; // 输入长度达到上限回调
public:
    WindKeyboard();
    virtual ~WindKeyboard();

    virtual void showKeyboard(const std::string& text = "", const std::string& hint = "");
    /// @brief 隐藏键盘
    /// @note 隐藏时会一并清空所有回调（enter/cancel/editChange），
    ///       请务必在每次 showKeyboard 之前重新设置回调，避免回调捕获的对象先被销毁
    virtual void hideKeyboard();
    bool         isKeyboardShow() const;

    void         setKeyboardMaxInputCount(int count);
    void         setKeyboardEditChangeCallBack(OnCloseListener listener);
    /// @brief 设置确认/取消回调
    /// @note 回调仅在本次显示期间有效，hideKeyboard 后自动清空，需在下次显示前重新设置
    void         setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel);
    /// @brief 设置输入长度达到上限的回调（用于提示用户，仅在设了长度上限时触发）
    /// @note 同 setKeyboardCallBack，隐藏后会自动清空
    void         setKeyboardMaxLengthCallBack(OnMaxLengthListener listener);

protected:
    void         init(ViewGroup* parent);
    bool         onKey(int keyCode, KeyEvent& evt, bool& result);

private:
    bool         checkInit();
    /// @brief 清空所有回调，避免持有已失效的调用方对象
    void         clearCallbacks();
    void         onKeyBoardFinish(bool isEnter, const std::string& text);
};

#endif // !__WIND_KEYBOARD_H__
