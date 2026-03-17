/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-03-18 01:18:52
 * @FilePath: /kk_frame/library/keyboard/include/cKeyBoard.h
 * @Description: 输入法 CDROID 版
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __C_KEYBOARD_H__
#define __C_KEYBOARD_H__

#include <set>
#include <vector>
#include <widget/button.h>
#include <widget/edittext.h>
#include <widget/relativelayout.h>

class CKeyBoardChild;

/// @brief 输入法 CDROID 版
class CKeyBoard : public RelativeLayout {
    friend CKeyBoardChild;
    
public:
    // 关闭回调
    typedef std::function<void(bool, const std::string&)> OnCloseListener;

    // 键盘类型
    typedef enum {
        KB_TYPE_NONE,

        KB_TYPE_EN,   // 英文
        KB_TYPE_CN,   // 中文

        KB_TYPE_MAX
    } KeyBoardType;

public:   // 构造
    CKeyBoard(int w, int h);
    CKeyBoard(Context* ctx, const AttributeSet& attr);
    ~CKeyBoard();

public:   // 外部用
    void show();
    void setType(KeyBoardType t);
    void setInputText(const std::string& txt);
    void setDescription(const std::string& txt);
    void setMaxInputCount(int count);
    void setEnableChilds(const std::vector<KeyBoardType>& childs);
    void setCloseListener(OnCloseListener closeListener);
    void setChineseWeight(int weight);

public:   // 子键盘用
    void appendText(const std::string& txt);
    void backspaceText();
    void showNextType();

private: // 内部用
    void init();
    void showType(KeyBoardType t);
    void setEditText(const std::string& txt);
    CKeyBoardChild* createChild(KeyBoardType t);

private:
    bool            mIsInit{ false };              // 是否已初始化

    KeyBoardType    mKBType{ KB_TYPE_NONE };       // 键盘加载类型
    std::string     mInputText{ "" };              // 输入框内容
    std::string     mDescription{ "" };            // 描述文本
    int             mMaxInputCount{ 20 };          // 最大输入长度
    OnCloseListener mCloseListener{ nullptr };     // 关闭回调
    int             mChineseWeight{ 2 };           // 中文字符权重

private:
    EditText*       mInputTextEdit{ nullptr };     // 输入框
    Button*         mCompleteBtn{ nullptr };       // 完成按钮
    Button*         mCancelBtn{ nullptr };         // 取消按钮
    ViewGroup*      mChildBox{ nullptr };          // 子键盘容器

    std::set<CKeyBoardChild*> mChilds;             // 子键盘
    std::vector<KeyBoardType> mEnableChilds;       // 启用的子键盘
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

protected:
    void updateParentBtn(const std::string& conplete, const std::string& cancel);
};

#endif // __C_KEYBOARD_H__