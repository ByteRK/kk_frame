/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-10 22:50:08
 * @LastEditTime: 2026-09-02 15:34:46
 * @FilePath: /kk_frame/src/app/page/components/wind_keyboard.cc
 * @Description: 键盘组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_keyboard.h"
#include "base.h"
#include "gauss_drawable.h"

#if DISABLED(KEYBOARD)
// 兜底策略，防止XML解析失败
class CKBegister {
public:
    CKBegister() {
        LayoutInflater::registerInflater("CKeyBoard", "", [](Context*ctx, const AttributeSet&attr)->View* {return new TextView(ctx, attr);});
    }
};
static CKBegister ckbegister;
#endif

WindKeyboard::WindKeyboard() { }

WindKeyboard::~WindKeyboard() {
    // 仅清理成员回调：此处不再触碰键盘视图（析构顺序不确定，视图可能已先释放）
    mEnterListener = nullptr;
    mCancelListener = nullptr;
}

/// @brief 显示键盘
/// @param text 初始文本
/// @param hint 提示文本
void WindKeyboard::showKeyboard(const std::string& text, const std::string& hint) {
    if (isKeyboardShow()) return;
    mIsShow = true;
    // 模糊只计算一次并缓存首帧，因此每次显示都重建，避免用到上一次显示时的模糊画面
    applyGauss();
    // 先显示再刷新内容：键盘内部会在"变为可见"时申请焦点，隐藏状态下申请会导致光标空转
    mKeyBoard->setVisibility(View::VISIBLE);
#if ENABLED(KEYBOARD)
    mKeyBoard->setInputText(text);
    mKeyBoard->setDescription(hint);
    mKeyBoard->setMaxInputCount(mMaxInputCount);
    mKeyBoard->show();
#else
    LOGE("Keyboard not enabled");
#endif
}

/// @brief 隐藏键盘
/// @note 隐藏时会一并清空所有回调（enter/cancel/editChange）
void WindKeyboard::hideKeyboard() {
    if (!isKeyboardShow()) return;
    mKeyBoard->setVisibility(View::GONE);
    mIsShow = false;
    // 回调可能捕获调用方（页面）的对象，键盘隐藏后立即释放，避免调用方先销毁后回调悬垂
    clearCallbacks();
    // 恢复默认输入长度（键盘库中 <=0 表示不限制，不能用 0 做复位），避免影响下一次调用者
    setKeyboardMaxInputCount(KEYBOARD_DEFAULT_INPUT_LIMIT);
}

/// @brief 键盘是否在显示中
/// @return 
bool WindKeyboard::isKeyboardShow()const {
    return mIsShow;
}

/// @brief 设置键盘输入长度上限
/// @param count 长度上限
void WindKeyboard::setKeyboardMaxInputCount(int count) {
    if (!checkInit()) return;
    mMaxInputCount = count;
#if ENABLED(KEYBOARD)
    mKeyBoard->setMaxInputCount(count);
#endif
}

/// @brief 设置输入框变化回调
/// @param listener 回调
void WindKeyboard::setKeyboardEditChangeCallBack(OnCloseListener listener) {
    if (!checkInit()) return;
#if ENABLED(KEYBOARD)
    mKeyBoard->setEditChangeListener(listener);
#endif
}

/// @brief 设置确认/取消回调
void WindKeyboard::setKeyboardCallBack(OnCloseListener enter, OnCloseListener cancel) {
    mEnterListener = enter;
    mCancelListener = cancel;
}

/// @brief 设置输入长度达到上限的回调
void WindKeyboard::setKeyboardMaxLengthCallBack(OnMaxLengthListener listener) {
    mMaxLengthListener = listener;
}

/// @brief 设置背景模糊
/// @param enable 是否启用，关闭时退化为纯色底
/// @param radius 模糊半径（越大越模糊）
/// @param color  模糊蒙版颜色
void WindKeyboard::setKeyboardGauss(bool enable, int radius, uint64_t color) {
    mGaussEnable = enable;
    mGaussRadius = radius;
    mGaussColor = color;
    if (isKeyboardShow()) applyGauss(); // 正在显示时立即生效
}

/// @brief 初始化
/// @param parent 
void WindKeyboard::init(ViewGroup* parent) {
    if (mIsInit) return;

    mKeyBoard = PBase::get<CKeyBoard>(parent, AppRid::keyboard);
    FailFast(mKeyBoard == nullptr, "WindKeyboard init failed");

    mKeyBoard->setVisibility(View::GONE);

#if ENABLED(KEYBOARD)
    // 键盘根布局：模糊背景的挂载对象（键盘库 XML 的半透明底色由它绘制，这里直接替换掉）
    mKeyBoardRoot = mKeyBoard->getRootView();
    FailFast(mKeyBoardRoot == nullptr, "WindKeyboard root init failed");

    mKeyBoard->setOnTouchListener([this](View&, MotionEvent&) {
        return true;
    });
    mKeyBoard->setSoundEffectsEnabled(false);
    mKeyBoard->setFinishListener([this](bool isEnter, const std::string& text) {
        onKeyBoardFinish(isEnter, text);
    });
    // 内部转发，实际回调由 setKeyboardMaxLengthCallBack 按需设置
    mKeyBoard->setMaxLengthListener([this](int maxCount) {
        if (mMaxLengthListener)mMaxLengthListener(maxCount);
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

/// @brief 键盘事件
/// @param keyCode 
/// @param evt 
/// @param result 
/// @return 
bool WindKeyboard::onKey(int keyCode, KeyEvent& evt, bool& result) {
    if (!isKeyboardShow() || evt.getAction() != KeyEvent::ACTION_DOWN) return false;
#if ENABLED(KEYBOARD)
    // 暂时不生效，会被editText抢占
    mKeyBoard->onRealKey(keyCode);
#endif
    return false;
}

/// @brief 检查是否已初始化
/// @return 
bool WindKeyboard::checkInit() {
    if (mIsInit) return true;
    LOGE("Keyboard uninit");
    return false;
}

/// @brief 清空所有回调
void WindKeyboard::clearCallbacks() {
    mEnterListener = nullptr;
    mCancelListener = nullptr;
    mMaxLengthListener = nullptr;
#if ENABLED(KEYBOARD)
    if (mIsInit && mKeyBoard)mKeyBoard->setEditChangeListener(nullptr);
#endif
}

/// @brief 键盘回调：键盘关闭时触发
/// @param isEnter 是否确认键关闭
/// @param text 输入内容
void WindKeyboard::onKeyBoardFinish(bool isEnter, const std::string& text) {
    // 先收起键盘（内部会清空回调），再触发本次回调的局部副本，
    // 这样回调里再次 showKeyboard/setKeyboardCallBack 不会被随后的清理误删
    OnCloseListener listener = isEnter ? mEnterListener : mCancelListener;
    hideKeyboard();
    if (listener) listener(text);
}

/// @brief 应用背景模糊
void WindKeyboard::applyGauss() {
    if (!mKeyBoardRoot) return; // 键盘库未启用或未初始化
#if ENABLED(GAUSS_DRAWABLE) || defined(__VSCODE__)
    if (mGaussEnable) {
        mKeyBoardRoot->setBackground(new GaussDrawable(mKeyBoardRoot, mGaussRadius, 0.5f, mGaussColor, true));
        return;
    }
    mKeyBoardRoot->setBackgroundColor(mGaussColor); // 关闭模糊时退化为纯色底
#else
    mKeyBoardRoot->setBackgroundColor(mGaussColor);
    LOGD("WindKeyboard::applyGauss() gauss drawable not enabled");
#endif
}
