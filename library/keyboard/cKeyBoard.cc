/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-03-18 10:30:43
 * @FilePath: /kk_frame/library/keyboard/cKeyBoard.cc
 * @Description: 输入法 CDROID 版
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "cKeyBoard.h"
#include "string_utils.h"
#include "custom_app.h"

#include "keyboard_en.h"
#include "keyboard_cn.h"

/********************************** 键盘外层 **********************************/

DECLARE_WIDGET(CKeyBoard)

CKeyBoard::CKeyBoard(int w, int h) : RelativeLayout(w, h) {
    init();
}

CKeyBoard::CKeyBoard(Context* ctx, const AttributeSet& attr) : RelativeLayout(ctx, attr) {
    init();
}

CKeyBoard::~CKeyBoard() {
    for (auto& kb : mChilds)
        delete kb;
    mChilds.clear();
}

void CKeyBoard::show() {
    setVisibility(View::VISIBLE);
    setEnabled(false);
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

void CKeyBoard::setChineseWeight(int weight) {
    mChineseWeight = weight;
}

void CKeyBoard::appendText(const std::string& txt) {
    std::string cacheText = mInputText + txt;
    if (mMaxInputCount && StringUtils::characterCount(cacheText.c_str(), mChineseWeight) > mMaxInputCount)
        cacheText = StringUtils::substringByChars(cacheText.c_str(), mMaxInputCount, mChineseWeight);
    setEditText(cacheText);
}

void CKeyBoard::backspaceText() {
    if (mInputText.empty())return;
    setEditText(StringUtils::removeLastCharacter(mInputText.c_str()));
}

void CKeyBoard::showNextType() {
    auto it = std::find(mEnableChilds.begin(), mEnableChilds.end(), mKBType);
    if (it == mEnableChilds.end()) {
        if (mEnableChilds.size() > 0) mKBType = mEnableChilds[0];
        else mKBType = KeyBoardType::KB_TYPE_NONE;
    } else {
        mKBType = (++it == mEnableChilds.end()) ? mEnableChilds[0] : *it;
    }
    showType(mKBType);
}

void CKeyBoard::init() {
    if (mIsInit)return;

    CustomApp* app = __dc(CustomApp, &CustomApp::getInstance());
    if (!app) throw std::runtime_error("main.cc must use CustomApp to replace cdroid::App !!!");

    if (!app->checkPackage("keyboard"))
        app->addPackage("./" PROJECT_NAME "_keyboard.pak", "keyboard");

    mKeyboardRoot = __dc(ViewGroup, LayoutInflater::from(getContext())->inflate("@keyboard:layout/keyboard", this));

    mInputTextEdit = __dc(EditText, mKeyboardRoot->findViewById(LibRid::input_box));
    mCompleteBtn = __dc(Button, mKeyboardRoot->findViewById(LibRid::enter));
    mCancelBtn = __dc(Button, mKeyboardRoot->findViewById(LibRid::cancel));
    mChildBox = __dc(ViewGroup, mKeyboardRoot->findViewById(LibRid::key_box));

    auto closeClick = [this](View&v) {
        if (mCloseListener)mCloseListener(v.getId() == LibRid::enter, mInputText);
        mInputText.clear();
        mDescription.clear();
        showType(KeyBoardType::KB_TYPE_NONE);
        setVisibility(View::GONE);
    };
    mCompleteBtn->setOnClickListener(closeClick);
    mCancelBtn->setOnClickListener(closeClick);

    setEnableChilds({ KB_TYPE_EN });

    setVisibility(View::GONE);

    mIsInit = true;
}

void CKeyBoard::showType(KeyBoardType t) {
    CKeyBoardChild* child_t = nullptr;
    for (auto& child : mChilds) {
        if (child->getType() == t) child_t = child;
        else child->onHide();
    }
    if (t == KB_TYPE_NONE || t == KB_TYPE_MAX) return;
    if (std::find(mEnableChilds.begin(), mEnableChilds.end(), t) == mEnableChilds.end()) {
        if (child_t)child_t->onHide();
        LOGW("keyboard type %d not allow show", t);
        return;
    }
    if (!child_t && !(child_t = createChild(t))) {
        LOGE("keyboard type %d can not create", t);
        return;
    }
    child_t->onShow();
}

void CKeyBoard::setEditText(const std::string& txt) {
    mInputText = txt;
    if (txt.empty()) {
        mInputTextEdit->setText(" " + mDescription);
        mInputTextEdit->setTextColor(App::getInstance().getColor("@keyboard:color/keyboard_color_description"));
        mInputTextEdit->setCaretPos(0);
        LOGD("setEditText: [%s]", mDescription.c_str());
    } else {
        std::string endText(txt + " ");
        int pos = StringUtils::characterCount(endText.c_str()) - 1;

        mInputTextEdit->setText(endText);
        mInputTextEdit->setTextColor(App::getInstance().getColor("@keyboard:color/keyboard_color_input"));
        mInputTextEdit->setCaretPos(pos);
        LOGI("setEditText: [CaretPos: %d][%s]", pos, txt.c_str());
    }
}

CKeyBoardChild* CKeyBoard::createChild(KeyBoardType t) {
    if (t == KB_TYPE_NONE || t == KB_TYPE_MAX)return nullptr;
    CKeyBoardChild* child = nullptr;

    switch (t) {
    case KB_TYPE_EN: { child = new Keyboard_EN(this); }break;
    case KB_TYPE_CN: { child = new Keyboard_CN(this); }break;
    default: break;
    }

    if (child) {
        child->init();
        mChilds.insert(child);
    }
    return child;
}

/******************************** 键盘内层基类 ********************************/

CKeyBoardChild::CKeyBoardChild(CKeyBoard* parent, const std::string& layout) :mParent(parent) {
    mRootView = __dc(ViewGroup, LayoutInflater::from(parent->getContext())->inflate(layout, parent->mChildBox));
    mRootView->setVisibility(View::GONE);
}

CKeyBoardChild::~CKeyBoardChild() {
}

void CKeyBoardChild::init() {
}

void CKeyBoardChild::onShow() {
    mRootView->setVisibility(View::VISIBLE);
}

void CKeyBoardChild::onHide() {
    mRootView->setVisibility(View::GONE);
}

void CKeyBoardChild::updateParentBtn(const std::string& conplete, const std::string& cancel) {
    mParent->mCompleteBtn->setText(conplete);
    mParent->mCancelBtn->setText(cancel);
}