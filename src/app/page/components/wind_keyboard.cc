/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:50:08
 * @LastEditTime: 2026-03-18 22:49:14
 * @FilePath: /kk_frame/src/app/page/components/wind_keyboard.cc
 * @Description: 键盘组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_keyboard.h"
#include "base.h"

#if !defined(ENABLE_KEYBOARD)
// 兜底策略，防止XML解析失败
class CKBegister {
public:
    CKBegister() {
        LayoutInflater::registerInflater("CKeyBoard", "", [](Context*ctx, const AttributeSet&attr)->View* {return new TextView(ctx, attr);});
    }
};
static CKBegister ckbegister;
#endif

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

    if (!(mKeyBoard = PBase::get<CKeyBoard>(parent, AppRid::keyboard)))
        throw std::runtime_error("WindKeyboard init failed");

    mKeyBoard->setVisibility(View::GONE);

#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setOnTouchListener([this](View&, MotionEvent&) {
        return true;
    });
    mKeyBoard->setFinishListener([this](bool isEnter, const std::string& text) {
        onKeyBoardFinish(isEnter, text);
    });
    mKeyBoard->setEnableChilds({ CKeyBoard::KB_TYPE_EN, CKeyBoard::KB_TYPE_CN });
    mKeyBoard->setType(CKeyBoard::KB_TYPE_EN);
#else
    mKeyBoard->setOnClickListener([this](View& view) {
        onKeyBoardFinish(false, "");
    });
    mKeyBoard->setBackgroundResource("#ff6b6b");
    mKeyBoard->setGravity(Gravity::CENTER);
    mKeyBoard->setTextColor(0xFFffffff);
    mKeyBoard->setTextSize(40);
    mKeyBoard->setText("KEYBOARD NOT ENABLE!!!");
#endif

    mIsInit = true;
}

void WindKeyboard::onTick() {
}

void WindKeyboard::showKeyboard(const std::string& text, const std::string& hint) {
    if (isKeyboardShow()) return;
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setInputText(text);
    mKeyBoard->setDescription(hint);
    mKeyBoard->show();
#else
    LOGE("Keyboard not enabled");
#endif
    mIsShow = true;
    mKeyBoard->setVisibility(View::VISIBLE);
}

void WindKeyboard::hideKeyboard() {
    if (!isKeyboardShow()) return;
    mKeyBoard->setVisibility(View::GONE);
    mIsShow = false;
}

bool WindKeyboard::isKeyboardShow() {
    return mIsShow;
}

bool WindKeyboard::onKey(int keyCode, KeyEvent& evt, bool& result) {
    if (!isKeyboardShow() || evt.getAction() != KeyEvent::ACTION_DOWN) return false;
#if defined(ENABLE_KEYBOARD)
    // 暂时不生效，会被editText抢占
    mKeyBoard->onRealKey(keyCode);
#endif
    return false;
}

void WindKeyboard::setKeyboardMaxInputCount(int count) {
    if (!checkInit()) return;
#if defined(ENABLE_KEYBOARD)
    mKeyBoard->setMaxInputCount(count);
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

void WindKeyboard::onKeyBoardFinish(bool isEnter, const std::string& text) {
    OnCloseListener listener = isEnter ? mEnterListener : mCancelListener;
    if (listener) listener(text);
    hideKeyboard();
}
