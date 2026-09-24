/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-22 00:00:00
 * @LastEditTime: 2026-09-22 00:00:00
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_qwerty.cc
 * @Description: QWERTY 类键盘基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_qwerty.h"

Keyboard_Qwerty::Keyboard_Qwerty(CKeyBoard* parent, const std::string& layout, int mainKeyCount)
    : CKeyBoardChild(parent, layout), mMainKeyCount(mainKeyCount) {
    mKeyList.reserve(mainKeyCount + TAIL_KEY_COUNT);
    mKeyStr.resize(DISPLAY_TYPE_MAX);
    mBackspaceTimer = std::bind(&Keyboard_Qwerty::onBackspaceTimer, this);
    LOGI("Keyboard_Qwerty::Keyboard_Qwerty() Created, mainKeyCount=%d", mainKeyCount);
}

Keyboard_Qwerty::~Keyboard_Qwerty() { }

void Keyboard_Qwerty::init() {
    mKeyList.clear();
    collectKeys();
    FailFast((int)mKeyList.size() != mMainKeyCount + TAIL_KEY_COUNT,
        "keyboard keys count mismatch: got %d, expect %d (check layout & key table)",
        (int)mKeyList.size(), mMainKeyCount + TAIL_KEY_COUNT);
    setKeyAction();
}

void Keyboard_Qwerty::onShow() {
    mRootView->setVisibility(View::VISIBLE);
    mDisplayType = DISPLAY_TYPE_DEFAULT;
    refreshDisplay();
}

void Keyboard_Qwerty::onHide() {
    mRootView->setVisibility(View::GONE);
    // 隐藏时取消未完成的长按判定，避免键盘收起后仍触发全删
    stopBackspaceTimer();
}

void Keyboard_Qwerty::setKeyAction() {
    // 用下标捕获，避免依赖控件ID（控件ID在包之间并不隔离，运行时改写会污染查找结果）
    for (size_t i = 0; i < mKeyList.size(); i++)
        mKeyList[i]->setOnClickListener([this, i](View&) { onKeyClick((int)i); });

    // 退格键：短按退一格；按住超过 BACKSPACE_LONG_PRESS_MS 直接全删
    mKeyList[idxBackspace()]->setOnTouchListener([this](View& v, MotionEvent& me) {
        switch (me.getActionMasked()) {
        case MotionEvent::ACTION_DOWN:
            mBackspaceHandled = false;
            v.setPressed(true);
            v.postDelayed(mBackspaceTimer, BACKSPACE_LONG_PRESS_MS);
            return true;
        case MotionEvent::ACTION_UP:
            v.setPressed(false);
            // 未触发长按，按普通点击处理（含点击音效）
            if (!mBackspaceHandled)v.performClick();
            stopBackspaceTimer();
            return true;
        case MotionEvent::ACTION_CANCEL:
            v.setPressed(false);
            stopBackspaceTimer();
            return true;
        default:
            return false;
        }
    });
}

void Keyboard_Qwerty::onBackspaceTimer() {
    mBackspaceHandled = true;
    onBackspaceLongPress();
}

void Keyboard_Qwerty::stopBackspaceTimer() {
    if (idxBackspace() < (int)mKeyList.size())mKeyList[idxBackspace()]->removeCallbacks(mBackspaceTimer);
    mBackspaceHandled = false;
}

void Keyboard_Qwerty::onBackspaceLongPress() {
    // 长按退格：清除光标前的内容（保留光标后的内容）
    mParent->clearBeforeCaret();
}

void Keyboard_Qwerty::onKeyClick(int key) {
    DISPLAY_TYPE typeTag = mDisplayType;
    // Shift键逻辑特殊处理
    if (key != idxShift()) {
        mShiftLastClickTime = 0;
        if (mDisplayType == DISPLAY_TYPE_UPPER)
            typeTag = DISPLAY_TYPE_DEFAULT;
    }

    if (isMainKey(key)) {
        // 常规按键
        mParent->appendText(key == idxSpace() ? " " : keyStr(key));
    } else if (key == idxMath()) {
        if (mDisplayType == DISPLAY_TYPE_NUMBER || mDisplayType == DISPLAY_TYPE_MORE)
            typeTag = DISPLAY_TYPE_DEFAULT;
        else
            typeTag = DISPLAY_TYPE_NUMBER;
    } else if (key == idxMore()) {
        if (mDisplayType == DISPLAY_TYPE_NUMBER)
            typeTag = DISPLAY_TYPE_MORE;
        else
            typeTag = DISPLAY_TYPE_NUMBER;
    } else if (key == idxShift()) {
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
    } else if (key == idxBackspace()) {
        mParent->backspaceText();
    } else if (key == idxLang()) {
        // 切语言后由目标子键盘自行刷新显示
        mParent->showNextType();
        return;
    } else {
        LOGE("Keyboard_Qwerty::onKeyClick() Unknown key: %d", key);
        return;
    }

    // 检查最终类型
    if (typeTag != mDisplayType) {
        mDisplayType = typeTag;
        refreshDisplay();
    }
}

void Keyboard_Qwerty::refreshDisplay() {
    updateKeyText();

    mKeyList[idxShift()]->setActivated(false);
    mKeyList[idxShift()]->setSelected(false);
    mKeyList[idxShift()]->setVisibility(View::VISIBLE);
    mKeyList[idxMore()]->setVisibility(View::GONE);

    switch (mDisplayType) {
    case DISPLAY_TYPE_UPPER: {
        mKeyList[idxShift()]->setActivated(true);
    }   break;
    case DISPLAY_TYPE_UPPER_PLUS: {
        mKeyList[idxShift()]->setSelected(true);
    }   break;
    case DISPLAY_TYPE_NUMBER: {
        mKeyList[idxShift()]->setVisibility(View::GONE);
        mKeyList[idxMore()]->setVisibility(View::VISIBLE);
    }   break;
    case DISPLAY_TYPE_MORE: {
        mKeyList[idxShift()]->setVisibility(View::GONE);
        mKeyList[idxMore()]->setVisibility(View::VISIBLE);
    }   break;
    default: {
    }   break;
    }
}

void Keyboard_Qwerty::setKeyStr(DISPLAY_TYPE type, std::initializer_list<const char*> keys) {
    if (type >= DISPLAY_TYPE_MAX)return;

    std::vector<std::string> row;
    row.reserve(keys.size());
    for (auto key : keys) row.push_back(key ? key : "");

    const size_t expect = static_cast<size_t>(mMainKeyCount) + TAIL_KEY_COUNT;
    if (row.size() != expect) {
        LOGE("keyboard key string count mismatch: got %d, expect %d (displayType=%d)",
            (int)row.size(), (int)expect, type);
        row.resize(expect);
    }
    mKeyStr[type] = std::move(row);
}

void Keyboard_Qwerty::copyKeyStr(DISPLAY_TYPE from, DISPLAY_TYPE to) {
    if (from >= DISPLAY_TYPE_MAX || to >= DISPLAY_TYPE_MAX)return;
    mKeyStr[to] = mKeyStr[from];
}

const std::string& Keyboard_Qwerty::keyStr(int key) const {
    static const std::string EMPTY_TEXT;
    if (mDisplayType >= mKeyStr.size())return EMPTY_TEXT;
    const std::vector<std::string>& row = mKeyStr[mDisplayType];
    return (key >= 0 && key < (int)row.size()) ? row[key] : EMPTY_TEXT;
}

void Keyboard_Qwerty::updateKeyText() {
    const size_t rowSize = (mDisplayType < mKeyStr.size()) ? mKeyStr[mDisplayType].size() : 0;
    for (size_t i = 0; i < mKeyList.size(); i++) {
        const std::string txt = (i < rowSize) ? mKeyStr[mDisplayType][i] : std::string();
        if (mKeyList[i]->getText() != txt)mKeyList[i]->setText(txt);
    }
}
