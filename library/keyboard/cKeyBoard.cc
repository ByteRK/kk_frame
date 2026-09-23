/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-08-10 09:57:43
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
#include "quick_define.h"

#include "keyboard_en.h"
#include "keyboard_cn.h"
#include "keyboard_ru.h"

/********************************** 键盘外层 **********************************/

DECLARE_WIDGET(CKeyBoard)

/// @brief 递归设置视图及子视图的音效开关（引擎的音效开关不向子视图继承）
/// @param view 目标视图
/// @param enabled 是否开启
static void applySoundEffects(View* view, bool enabled) {
    if (!view)return;
    view->setSoundEffectsEnabled(enabled);
    ViewGroup* group = __dc(ViewGroup, view);
    if (!group)return;
    for (int i = 0; i < group->getChildCount(); i++)
        applySoundEffects(group->getChildAt(i), enabled);
}

CKeyBoard::CKeyBoard(int w, int h) : RelativeLayout(w, h) {
    init();
}

CKeyBoard::CKeyBoard(Context* ctx, const AttributeSet& attr) : RelativeLayout(ctx, attr) {
    init();
}

CKeyBoard::~CKeyBoard() {
    // 先摘除子键盘视图，避免视图上的点击回调仍指向即将释放的子键盘对象
    if (mChildBox)mChildBox->removeAllViews();
    mCurChild = nullptr;
    for (auto& kv : mChilds) delete kv.second;
    mChilds.clear();
}

void CKeyBoard::show() {
    // setVisibility(View::VISIBLE); // 交给外部控制
    showType(mKBType);
    setEditText(mInputText);
    setInputBoxFocus(true);
}

void CKeyBoard::setType(KeyBoardType t) {
    if (t != KB_TYPE_NONE && !isTypeValid(t)) {
        LOGW("keyboard type %d is invalid, ignore", t);
        return;
    }
    mKBType = t;
}

void CKeyBoard::setInputText(const std::string& txt) {
    mInputText = txt;
}

void CKeyBoard::setDescription(const std::string& txt) {
    mDescription = txt;
}

/// @note count <= 0 表示不限制输入长度
void CKeyBoard::setMaxInputCount(int count) {
    mMaxInputCount = count;
}

void CKeyBoard::setEnableChilds(const std::vector<KeyBoardType>& childs) {
    mEnableChilds.clear();
    for (auto t : childs) {
        if (!isTypeValid(t)) {
            LOGW("keyboard type %d is invalid, skip", t);
            continue;
        }
        if (isTypeEnable(t))continue;
#if !ENABLED(KEYBOARD_PINYIN)
        if (t == KB_TYPE_CN) {
            LOGW("keyboard type CN needs ENABLE_KEYBOARD_PINYIN, skip");
            continue;
        }
#endif
        mEnableChilds.push_back(t);
    }
    // 当前类型不可用时回退到首个可用类型（KB_TYPE_NONE 表示未指定，保持原样）
    if (isTypeValid(mKBType) && !isTypeEnable(mKBType))
        mKBType = mEnableChilds.empty() ? KB_TYPE_NONE : mEnableChilds.front();
}

void CKeyBoard::setFinishListener(OnFinishListener finishListener) {
    mFinishListener = finishListener;
}

void CKeyBoard::setEditChangeListener(OnEditChangeListener editChangeListener) {
    mEditChangeListener = editChangeListener;
}

void CKeyBoard::setMaxLengthListener(OnMaxLengthListener maxLengthListener) {
    mMaxLengthListener = maxLengthListener;
}

void CKeyBoard::setChineseWeight(int weight) {
    mChineseWeight = weight;
}

void CKeyBoard::setSoundEffectsEnabled(bool enabled) {
    // 注意：本函数名与 View::setSoundEffectsEnabled 同名（非虚），
    // 仅对持有 CKeyBoard* 的调用生效，用于补上引擎"不向子视图继承音效开关"的行为
    RelativeLayout::setSoundEffectsEnabled(enabled);
    mSoundEffects = enabled;
    applySoundEffects(mKeyboardRoot, enabled);
    for (auto& kv : mChilds) kv.second->setSoundEffectsEnabled(enabled);
}

void CKeyBoard::onRealKey(int keyCode) {
    if (mCurChild)mCurChild->onRealKey(keyCode);
}

void CKeyBoard::appendText(const std::string& txt) {
    std::string cacheText = mInputText + txt;
    bool truncated = false;
    if (mMaxInputCount > 0 && StringUtils::characterCount(cacheText.c_str(), mChineseWeight) > mMaxInputCount) {
        cacheText = StringUtils::substringByChars(cacheText.c_str(), mMaxInputCount, mChineseWeight);
        truncated = true;
    }
    if (cacheText != mInputText)setEditText(cacheText);
    // 触顶提示放在最后，避免回调内再次输入造成重入
    if (truncated && mMaxLengthListener)mMaxLengthListener(mMaxInputCount);
}

void CKeyBoard::backspaceText() {
    if (mInputText.empty())return;
    setEditText(StringUtils::removeLastCharacter(mInputText.c_str()));
}

void CKeyBoard::clearText() {
    if (mInputText.empty())return;
    setEditText("");
}

void CKeyBoard::showNextType() {
    if (mEnableChilds.empty()) {
        mKBType = KB_TYPE_NONE;
    } else {
        auto it = std::find(mEnableChilds.begin(), mEnableChilds.end(), mKBType);
        if ((it == mEnableChilds.end()) || (++it == mEnableChilds.end()))
            mKBType = mEnableChilds.front();
        else mKBType = *it;
    }
    showType(mKBType);
}

void CKeyBoard::setBtnText(const std::string& complete, const std::string& cancel) {
    if (mCompleteBtn)mCompleteBtn->setText(complete);
    if (mCancelBtn)mCancelBtn->setText(cancel);
}

void CKeyBoard::onVisibilityChanged(View& changedView, int visibility) {
    RelativeLayout::onVisibilityChanged(changedView, visibility);
    // 引擎只在本视图自身 GONE 时清理焦点，祖先隐藏时子视图仍持有焦点（光标空转、IMM 不释放）
    setInputBoxFocus(isShown());
}

void CKeyBoard::init() {
    if (mIsInit)return;

    CustomApp* app = __dc(CustomApp, &CustomApp::getInstance());
    FailFast(!app, "main.cc must use CustomApp to replace cdroid::App !!!");

    if (!app->checkPackage("keyboard"))
        app->addPackage("./" PROJECT_NAME "_keyboard.pak", "keyboard");

    mKeyboardRoot = __dc(ViewGroup, LayoutInflater::from(getContext())->inflate("@keyboard:layout/keyboard", this));

    mInputTextEdit = __dc(EditText, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::input_box) : nullptr);
    mCompleteBtn = __dc(Button, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::enter) : nullptr);
    mCancelBtn = __dc(Button, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::cancel) : nullptr);
    mChildBox = __dc(ViewGroup, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::key_box) : nullptr);
    FailFast(!mKeyboardRoot || !mInputTextEdit || !mCompleteBtn || !mCancelBtn || !mChildBox,
        "CKeyBoard init failed, check @keyboard:layout/keyboard and the ids in R.h");

    mInputColor = App::getInstance().getColor("@keyboard:color/keyboard_color_input");
    mDescriptionColor = App::getInstance().getColor("@keyboard:color/keyboard_color_description");
    if (!mInputColor)mInputColor = 0xFFF9F9F9;
    if (!mDescriptionColor)mDescriptionColor = 0x88F9F9F9;

    // 输入框仅用于显示内容：屏蔽触摸，避免用户拖动光标/选中文本后与 mInputText 失步
    mInputTextEdit->setOnTouchListener([](View&, MotionEvent&) { return true; });

    auto btnClick = [this](View&v) {
        if (mFinishListener)mFinishListener(v.getId() == LibRid::enter, mInputText);
        mInputText.clear();
        mDescription.clear();
        showType(KeyBoardType::KB_TYPE_NONE);
        // setVisibility(View::GONE); // 交给外部控制
    };
    mCompleteBtn->setOnClickListener(btnClick);
    mCancelBtn->setOnClickListener(btnClick);

    setEnableChilds({ KB_TYPE_EN });

    mIsInit = true;
}

void CKeyBoard::showType(KeyBoardType t) {
    auto it = mChilds.find(t);
    CKeyBoardChild* child_t = (it == mChilds.end()) ? nullptr : it->second;
    for (auto& kv : mChilds) {
        if (kv.first != t)kv.second->onHide();
    }

    if (!child_t) {
        mCurChild = nullptr;
        if (isTypeEnable(t) && !(child_t = createChild(t))) {
            LOGE("keyboard type %d can not create", t);
            return;
        }
        if (!child_t) {
            if (isTypeValid(t))LOGW("keyboard type %d not allow show", t);
            return;
        }
    }

    mCurChild = child_t;
    child_t->onShow();
}

void CKeyBoard::setEditText(const std::string& txt) {
    mInputText = txt;
    if (txt.empty()) {
        mInputTextEdit->setText(" " + mDescription);
        mInputTextEdit->setTextColor(mDescriptionColor);
        mInputTextEdit->setCaretPos(0);
        LOGD("setEditText: [%s]", mDescription.c_str());
    } else {
        std::string endText(txt + " ");
        int pos = StringUtils::characterCount(endText.c_str()) - 1;

        mInputTextEdit->setText(endText);
        mInputTextEdit->setTextColor(mInputColor);
        mInputTextEdit->setCaretPos(pos);
        LOGI("setEditText: [CaretPos: %d][%s]", pos, txt.c_str());
    }

    if (mEditChangeListener)mEditChangeListener(mInputText);
}

void CKeyBoard::setInputBoxFocus(bool focus) {
    if (!mInputTextEdit)return;
    if (focus) {
        if (isShown())mInputTextEdit->requestFocus();
    } else if (mInputTextEdit->isFocused()) {
        mInputTextEdit->clearFocus();
    }
}

CKeyBoardChild* CKeyBoard::createChild(KeyBoardType t) {
    CKeyBoardChild* child = nullptr;

    switch (t) {
    case KB_TYPE_EN: { child = new Keyboard_EN(this); }break;
    case KB_TYPE_CN: { child = new Keyboard_CN(this); }break;
    case KB_TYPE_RU: { child = new Keyboard_RU(this); }break;
    default: break;
    }

    if (!child)return nullptr;
    child->init();
    child->setSoundEffectsEnabled(mSoundEffects);
    mChilds[t] = child;
    return child;
}

bool CKeyBoard::isTypeEnable(KeyBoardType t) const {
    return std::find(mEnableChilds.begin(), mEnableChilds.end(), t) != mEnableChilds.end();
}

bool CKeyBoard::isTypeValid(KeyBoardType t) {
    return t > KB_TYPE_NONE && t < KB_TYPE_MAX;
}

/******************************** 键盘内层基类 ********************************/

CKeyBoardChild::CKeyBoardChild(CKeyBoard* parent, const std::string& layout) :mParent(parent) {
    mRootView = __dc(ViewGroup, LayoutInflater::from(parent->getContext())->inflate(layout, parent->mChildBox));
    FailFast(!mRootView, "CKeyBoardChild inflate failed: %s", layout.c_str());
    mRootView->setVisibility(View::GONE);
}

CKeyBoardChild::~CKeyBoardChild() {
    mRootView = nullptr;  // 视图由父容器管理，此处仅断开引用
}

void CKeyBoardChild::init() { }

void CKeyBoardChild::onShow() {
    mRootView->setVisibility(View::VISIBLE);
}

void CKeyBoardChild::onHide() {
    mRootView->setVisibility(View::GONE);
}

void CKeyBoardChild::onRealKey(int keyCode) { }

void CKeyBoardChild::setSoundEffectsEnabled(bool enabled) {
    applySoundEffects(mRootView, enabled);
}

void CKeyBoardChild::updateParentBtn(const std::string& conplete, const std::string& cancel) {
    mParent->setBtnText(conplete, cancel);
}