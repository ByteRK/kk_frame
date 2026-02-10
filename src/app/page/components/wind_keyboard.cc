/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:50:08
 * @LastEditTime: 2026-02-11 01:05:51
 * @FilePath: /kk_frame/src/app/page/components/wind_keyboard.cc
 * @Description: 键盘组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_keyboard.h"
#include "base.h"

WindKeyboard::WindKeyboard() {
    mIsInit = false;
    mIsShow = false;
    mEnterListener = nullptr;
    mCancelListener = nullptr;
}

WindKeyboard::~WindKeyboard() {
}

void WindKeyboard::init(ViewGroup* parent) {
    if (mIsInit) return;

    if (
        !(mRootView = PBase::get<ViewGroup>(parent, AppRid::keyboard_box))
#if defined(ENABLE_KEYBOARD)
        || !(mKeyBoard = PBase::get<CKeyBoard>(mRootView, AppRid::keyboard))
#endif
        )
        throw std::runtime_error("WindKeyboard init failed");

    mRootView->setOnTouchListener([this](View&, MotionEvent&) {
        return true;
    });

    mRootView->setVisibility(View::GONE);
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setOnCompleteListener([this](const std::string& text) {
        this->onKeyBoardClose(true, text);
    });
    mKeyBoard->setOnCancelListener([this]() {
        this->onKeyBoardClose(false, "");
    });
    mKeyBoard->setEditType(EditText::TYPE_ANY);
    mKeyBoard->setWordCount(30);
#endif

    mIsInit = true;
}

void WindKeyboard::onTick() {
}

void WindKeyboard::showKeyboard(const std::string& text, const std::string& hint) {
    if (isKeyboardShow()) return;
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->showWindow();
    mKeyBoard->setDescription(hint);
    mKeyBoard->setEditText(text);
    mRootView->setVisibility(View::VISIBLE);
    mIsShow = true;
#else
    LOGE("Keyboard not enabled");
#endif
}

void WindKeyboard::hideKeyboard() {
    if (!isKeyboardShow()) return;
    mRootView->setVisibility(View::GONE);
    mIsShow = false;
}

bool WindKeyboard::isKeyboardShow() {
    return mIsShow;
}

bool WindKeyboard::onKey(int keyCode, KeyEvent& evt, bool& result) {
    return false;
}

void WindKeyboard::setKeyboardWordCount(int count) {
    if (!checkInit()) return;
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setWordCount(count);
#endif
}

void WindKeyboard::setKeyboardType(cdroid::EditText::INPUTTYPE type) {
    if (!checkInit()) return;
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setEditType(type);
#endif
}

void WindKeyboard::setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel) {
    mEnterListener = enter;
    mCancelListener = cancel;
}

inline bool WindKeyboard::checkInit() {
    if (mIsInit) return true;
    LOGE("Keyboard uninit");
    return false;
}

void WindKeyboard::onKeyBoardClose(bool isEnter, const std::string& text) {
    OnCloseListener listener = isEnter ? mEnterListener : mCancelListener;
    if (listener) listener(text);
    hideKeyboard();
}
