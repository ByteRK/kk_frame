/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-09-23 14:59:13
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
#include "keyboard_edittext.h"

#include <core/textutils.h>

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
    mCaretIndex = txt.size();  // 外部灌入内容时光标置于末尾
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
    if (txt.empty())return;

    // 在光标处插入：光标按 utf8 字节偏移保存，这里统一用宽字符做插入，避免切在半个字符上
    std::wstring wideText = cdroid::TextUtils::utf8tounicode(mInputText);
    const int wideCaret = (int)cdroid::TextUtils::utf8tounicode(mInputText.substr(0, mCaretIndex)).size();
    const std::wstring wideIns = cdroid::TextUtils::utf8tounicode(txt);

    wideText.insert(wideCaret, wideIns);
    std::string result = cdroid::TextUtils::unicode2utf8(wideText);

    bool truncated = false;
    if (mMaxInputCount > 0 && StringUtils::characterCount(result.c_str(), mChineseWeight) > mMaxInputCount) {
        result = StringUtils::substringByChars(result.c_str(), mMaxInputCount, mChineseWeight);
        truncated = true;
    }

    // 光标停在插入内容之后（截断后按结果长度收敛到字符边界）
    mInputText = result;
    const std::wstring wideResult = cdroid::TextUtils::utf8tounicode(result);
    int newWideCaret = wideCaret + (int)wideIns.size();
    if (newWideCaret > (int)wideResult.size())newWideCaret = (int)wideResult.size();
    mCaretIndex = (int)cdroid::TextUtils::unicode2utf8(wideResult.substr(0, newWideCaret)).size();

    setEditText(mInputText);
    // 触顶提示放在最后，避免回调内再次输入造成重入
    if (truncated && mMaxLengthListener)mMaxLengthListener(mMaxInputCount);
}

void CKeyBoard::backspaceText() {
    // 删除光标左侧的字符（光标感知）
    if (mInputText.empty() || mCaretIndex <= 0)return;

    std::wstring wideText = cdroid::TextUtils::utf8tounicode(mInputText);
    const int wideCaret = (int)cdroid::TextUtils::utf8tounicode(mInputText.substr(0, mCaretIndex)).size();
    if (wideCaret <= 0)return;

    wideText.erase(wideCaret - 1, 1);
    mInputText = cdroid::TextUtils::unicode2utf8(wideText);
    mCaretIndex = (int)cdroid::TextUtils::unicode2utf8(wideText.substr(0, wideCaret - 1)).size();

    setEditText(mInputText);
}

void CKeyBoard::clearBeforeCaret() {
    // 删除光标前的内容，光标移到最前，光标后的内容保留
    if (mInputText.empty() || mCaretIndex <= 0)return;

    mInputText = mInputText.substr(mCaretIndex);
    mCaretIndex = 0;
    setEditText(mInputText);
}

void CKeyBoard::clearAllText() {
    // 内容为空时也需通知子键盘清理临时状态（如残留的候选栏）
    if (!mInputText.empty()) {
        mInputText.clear();
        mCaretIndex = 0;
        setEditText(mInputText);
    }
    // 通知当前子键盘清理临时状态（如中文候选、拼音）
    if (mCurChild)mCurChild->onTextCleared();
}

void CKeyBoard::setCaretIndex(int index) {
    if (index < 0)index = 0;
    else if (index > (int)mInputText.size())index = mInputText.size();
    if (index == mCaretIndex)return;

    mCaretIndex = index;
    // 只移动光标，内容不变
    if (mInputTextEdit) {
        const std::wstring widePrefix = cdroid::TextUtils::utf8tounicode(mInputText.substr(0, mCaretIndex));
        mInputTextEdit->setCaretOffset((int)widePrefix.size());
    }
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

    mInputTextEdit = __dc(KeyboardEditText, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::input_box) : nullptr);
    mClearBtn = __dc(ImageView, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::clear) : nullptr);
    mCompleteBtn = __dc(Button, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::enter) : nullptr);
    mCancelBtn = __dc(Button, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::cancel) : nullptr);
    mChildBox = __dc(ViewGroup, mKeyboardRoot ? mKeyboardRoot->findViewById(LibRid::key_box) : nullptr);
    FailFast(!mKeyboardRoot || !mInputTextEdit || !mClearBtn || !mCompleteBtn || !mCancelBtn || !mChildBox,
        "CKeyBoard init failed, check @keyboard:layout/keyboard (input_box needs KeyboardEditText) and the ids in R.h");

    mInputColor = App::getInstance().getColor("@keyboard:color/keyboard_color_input");
    mDescriptionColor = App::getInstance().getColor("@keyboard:color/keyboard_color_description");
    if (!mInputColor)mInputColor = 0xFFF9F9F9;
    if (!mDescriptionColor)mDescriptionColor = 0x88F9F9F9;

    // 点击输入框可改光标位置（插入点），光标变化后同步回 mCaretIndex
    mInputTextEdit->setOnCaretChangeListener([this](int wideOffset) {
        onInputCaretChanged(wideOffset);
    });

    // 清除按键：点击即清空全部内容
    mClearBtn->setOnClickListener([this](View&) {
        clearAllText();
    });
    mClearBtn->getDrawable()->setFilterBitmap(true);

    auto btnClick = [this](View&v) {
        if (mFinishListener)mFinishListener(v.getId() == LibRid::enter, mInputText);
        mInputText.clear();
        mCaretIndex = 0;
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
    if (mCaretIndex > (int)mInputText.size())mCaretIndex = mInputText.size();
    syncEditText();
}

void CKeyBoard::syncEditText() {
    if (mInputText.empty()) {
        mInputTextEdit->setText(" " + mDescription);
        mInputTextEdit->setTextColor(mDescriptionColor);
        mInputTextEdit->setCaretOffset(0);
        LOGD("setEditText: [%s]", mDescription.c_str());
    } else {
        mInputTextEdit->setText(mInputText);
        mInputTextEdit->setTextColor(mInputColor);
        // 光标位置（utf8 字节偏移 -> 宽字符索引）
        const std::wstring widePrefix = cdroid::TextUtils::utf8tounicode(mInputText.substr(0, mCaretIndex));
        const int wideCaret = (int)widePrefix.size();
        mInputTextEdit->setCaretOffset(wideCaret);
        LOGI("setEditText: [Caret: %d][%s]", wideCaret, mInputText.c_str());
    }

    if (mEditChangeListener)mEditChangeListener(mInputText);
}

void CKeyBoard::onInputCaretChanged(int wideOffset) {
    // 输入框点击改了光标：宽字符索引 -> utf8 字节偏移
    const std::wstring wideText = cdroid::TextUtils::utf8tounicode(mInputText);
    int wideCaret = wideOffset;
    if (wideCaret < 0)wideCaret = 0;
    else if (wideCaret > (int)wideText.size())wideCaret = (int)wideText.size();

    mCaretIndex = (int)cdroid::TextUtils::unicode2utf8(wideText.substr(0, wideCaret)).size();
    LOGD("onInputCaretChanged: wide=%d -> utf8=%d", wideCaret, mCaretIndex);
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

void CKeyBoardChild::onTextCleared() { }

void CKeyBoardChild::setSoundEffectsEnabled(bool enabled) {
    applySoundEffects(mRootView, enabled);
}

void CKeyBoardChild::updateParentBtn(const std::string& conplete, const std::string& cancel) {
    mParent->setBtnText(conplete, cancel);
}