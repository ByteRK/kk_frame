/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-03-16 23:58:13
 * @FilePath: /kk_frame/library/keyboard/cKeyBoard.cc
 * @Description: 输入法 CDROID 版
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "R.h"
#include "cKeyBoard.h"
#include "app_version.h"
#include "string_utils.h"

namespace AppRid = APP_NAME::R::id;

#ifndef __dc
#define __dc(type, obj) dynamic_cast<type*>(obj)
#endif

/******************************** 键盘内层基类 ********************************/

CKeyBoard::CKeyBoardChild::CKeyBoardChild(CKeyBoard* parent, const std::string& layout) :mParent(parent) {

}

void CKeyBoard::CKeyBoardChild::onShow() {
}

void CKeyBoard::CKeyBoardChild::onHide() {
}

void CKeyBoard::CKeyBoardChild::updateParentBtn(const std::string& conplete, const std::string& cancel) {
    mParent->mCompleteBtn->setText(conplete);
    mParent->mCancelBtn->setText(cancel);
}

/********************************** 键盘外层 **********************************/

DECLARE_WIDGET(CKeyBoard)

CKeyBoard::CKeyBoard(int w, int h) : RelativeLayout(w, h) {
    init();
}

CKeyBoard::CKeyBoard(Context* ctx, const AttributeSet& attr) : RelativeLayout(ctx, attr) {
    init();
}

void CKeyBoard::show() {
    setVisibility(View::VISIBLE);
    showType(mKBType); // 显示当前键盘
    setEditText(mInputText);
    mInputTextEdit->requestFocus();
}

void CKeyBoard::setType(KeyBoardType t) {
    mKBType = t;
}

void CKeyBoard::setInputText(const std::string& txt) {
    mInputText = txt;
}

void CKeyBoard::setDescription(const std::string& txt) {
    mDescription = txt;
}

void CKeyBoard::setMaxInputCount(int count) {
    mMaxInputCount = count;
}

void CKeyBoard::setEnableChilds(const std::vector<KeyBoardType>& childs) {
    mEnableChilds = childs;
}

void CKeyBoard::setCloseListener(OnCloseListener closeListener) {
    mCloseListener = closeListener;
}

void CKeyBoard::registerChild(CKeyBoardChild* child) {
    mChilds.insert(child);
}

void CKeyBoard::appendText(const std::string& txt) {
}

void CKeyBoard::backspaceText() {
}

void CKeyBoard::showNextType() {
}

void CKeyBoard::init() {
    if (mIsInit)return;
    LayoutInflater::from(getContext())->inflate("@layout/keyboard", this);

    mInputTextEdit = __dc(EditText, findViewById(AppRid::input_box));
    mCompleteBtn = __dc(Button, findViewById(AppRid::enter));
    mCancelBtn = __dc(Button, findViewById(AppRid::cancel));
    mChildBox = __dc(ViewGroup, findViewById(AppRid::key_box));


    auto closeClick = [this](View&v) {
        if (mCloseListener)mCloseListener(v.getId() == AppRid::enter, mInputText);
        mKBType = KeyBoardType::KB_TYPE_NONE;
        mInputText.clear();
        mDescription.clear();
        setVisibility(View::GONE);
    };
    mCompleteBtn->setOnClickListener(closeClick);
    mCancelBtn->setOnClickListener(closeClick);

    setVisibility(View::GONE);

    mIsInit = true;
}

void CKeyBoard::showType(KeyBoardType t) {
}

void CKeyBoard::setEditText(const std::string& txt) {
    mInputText = txt;
    if (txt.empty()) {
        mInputTextEdit->setText(" " + mDescription);
        mInputTextEdit->setCaretPos(0);
        mInputTextEdit->setAlpha(0.5f);
    } else {
        mInputTextEdit->setText(txt + " ");
        mInputTextEdit->setCaretPos(txt.length());
        mInputTextEdit->setAlpha(1.0f);
    }
}
