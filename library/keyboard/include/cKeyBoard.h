/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-08-10 09:53:10
 * @FilePath: /kk_frame/library/keyboard/include/cKeyBoard.h
 * @Description: 输入法 CDROID 版
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __C_KEYBOARD_H__
#define __C_KEYBOARD_H__

#include <map>
#include <vector>
#include <widget/button.h>
#include <widget/edittext.h>
#include <widget/relativelayout.h>

class CKeyBoardChild;

/// @brief 输入法 CDROID 版
class CKeyBoard : public RelativeLayout {
    friend CKeyBoardChild;

public:
    // 完成回调
    typedef std::function<void(bool, const std::string&)> OnFinishListener;
    // 输入框内容改变回调
    typedef std::function<void(const std::string&)>       OnEditChangeListener;
    // 输入长度达到上限回调（参数为上限值）
    typedef std::function<void(int)>                      OnMaxLengthListener;

    // 键盘类型
    typedef enum {
        KB_TYPE_NONE,

        KB_TYPE_EN,   // 英文
        KB_TYPE_CN,   // 中文
        KB_TYPE_RU,   // 俄文

        KB_TYPE_MAX
    } KeyBoardType;

    // 默认最大输入长度（setMaxInputCount 传 0 或负数表示不限制）
    static constexpr int DEFAULT_INPUT_LIMIT = 20;

public:   // 构造
    CKeyBoard(int w, int h);
    CKeyBoard(Context* ctx, const AttributeSet& attr);
    ~CKeyBoard();

public:   // 外部用
    void show();
    void setType(KeyBoardType t);
    void setInputText(const std::string& txt);
    void setDescription(const std::string& txt);
    /// @brief 设置最大输入长度
    /// @note count <= 0 表示不限制长度（注意与"重置为默认值"的区别，默认值见 DEFAULT_INPUT_LIMIT）
    void setMaxInputCount(int count);
    void setEnableChilds(const std::vector<KeyBoardType>& childs);
    void setFinishListener(OnFinishListener finishListener);
    void setEditChangeListener(OnEditChangeListener editChangeListener);
    /// @brief 设置输入长度达到上限的回调（仅在设了长度上限时有效）
    void setMaxLengthListener(OnMaxLengthListener maxLengthListener);
    void setChineseWeight(int weight);
    /// @brief 按键音效开关（注意：引擎的音效开关不会向子视图继承，这里统一应用到整棵键盘视图）
    void setSoundEffectsEnabled(bool enabled);

public:   // 真实按键
    void onRealKey(int keyCode);

public:   // 子键盘用
    void appendText(const std::string& txt);
    void backspaceText();
    /// @brief 清空输入内容（退格键长按使用）
    void clearText();
    void showNextType();
    /// @brief 设置确认/取消按钮文案（子键盘可借此按自身语言定制）
    void setBtnText(const std::string& complete, const std::string& cancel);

protected:
    /// @brief 可见性变化：引擎只在自身 GONE 时清理焦点，这里补齐"祖先隐藏"与"重新显示"的焦点处理
    void onVisibilityChanged(View& changedView, int visibility) override;

private: // 内部用
    void init();
    void showType(KeyBoardType t);
    void setEditText(const std::string& txt);
    void setInputBoxFocus(bool focus);
    CKeyBoardChild* createChild(KeyBoardType t);
    bool isTypeEnable(KeyBoardType t) const;
    static bool isTypeValid(KeyBoardType t);

private:
    bool                 mIsInit{ false };                          // 是否已初始化

    KeyBoardType         mKBType{ KB_TYPE_NONE };                   // 键盘加载类型
    std::string          mInputText{ "" };                          // 输入框内容
    std::string          mDescription{ "" };                        // 描述文本
    int                  mMaxInputCount{ DEFAULT_INPUT_LIMIT };     // 最大输入长度（<=0 不限制）
    OnFinishListener     mFinishListener{ nullptr };                // 完成回调
    OnEditChangeListener mEditChangeListener{ nullptr };            // 输入框内容改变回调
    OnMaxLengthListener  mMaxLengthListener{ nullptr };             // 输入长度达到上限回调
    int                  mChineseWeight{ 2 };                       // 中文字符权重
    bool                 mSoundEffects{ true };                     // 按键音效开关，新建子键盘时同步
    int                  mInputColor{ 0 };                          // 输入框文字颜色（缓存，避免每键解析）
    int                  mDescriptionColor{ 0 };                    // 描述文字颜色（缓存）

private:
    ViewGroup*       mKeyboardRoot{ nullptr };      // 键盘根布局

    EditText*        mInputTextEdit{ nullptr };     // 输入框
    Button*          mCompleteBtn{ nullptr };       // 确认按钮
    Button*          mCancelBtn{ nullptr };         // 取消按钮
    ViewGroup*       mChildBox{ nullptr };          // 子键盘容器

    CKeyBoardChild*  mCurChild{ nullptr };          // 当前子键盘

    std::map<KeyBoardType, CKeyBoardChild*> mChilds;   // 子键盘（类型 -> 实例）
    std::vector<KeyBoardType> mEnableChilds;           // 启用的子键盘
};

/// @brief 子键盘基类
class CKeyBoardChild {
protected:
    CKeyBoard*const mParent;
    ViewGroup*      mRootView{ nullptr };

public:
    CKeyBoardChild(CKeyBoard* parent, const std::string& layout);
    virtual ~CKeyBoardChild();
    virtual CKeyBoard::KeyBoardType getType() = 0;
    virtual void init();
    virtual void onShow();
    virtual void onHide();
    virtual void onRealKey(int keyCode);
    /// @brief 按键音效开关（应用到本子键盘视图）
    void setSoundEffectsEnabled(bool enabled);

protected:
    void updateParentBtn(const std::string& conplete, const std::string& cancel);
};

#endif // __C_KEYBOARD_H__