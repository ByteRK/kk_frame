/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-16 16:03:05
 * @LastEditTime: 2026-03-16 23:50:22
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

class CKeyBoard : public RelativeLayout {
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

public:
    class CKeyBoardChild {
    protected:
        template<typename T = View>
        T* __get(int id) {
            return dynamic_cast<T*>(mRootView->findViewById(id));
        }

    protected:
        const CKeyBoard* mParent;
        ViewGroup*       mRootView{ nullptr };

    public:
        CKeyBoardChild(CKeyBoard* parent, const std::string& layout);
        virtual KeyBoardType getType() = 0;
        virtual void onShow();
        virtual void onHide();

    protected:
        void updateParentBtn(const std::string& conplete, const std::string& cancel);
    };
    friend class CKeyBoardChild;

public:   // 构造
    CKeyBoard(int w, int h);
    CKeyBoard(Context* ctx, const AttributeSet& attr);

public:   // 外部用
    void show();
    void setType(KeyBoardType t);
    void setInputText(const std::string& txt);
    void setDescription(const std::string& txt);
    void setMaxInputCount(int count);
    void setEnableChilds(const std::vector<KeyBoardType>& childs);
    void setCloseListener(OnCloseListener closeListener);

public:   // 子键盘用
    void registerChild(CKeyBoardChild* child);
    void appendText(const std::string& txt);
    void backspaceText();
    void showNextType();

private: // 内部用
    void init();
    void showType(KeyBoardType t);
    void setEditText(const std::string& txt);

private:
    bool            mIsInit{ false };              // 是否已初始化

    KeyBoardType    mKBType{ KB_TYPE_EN };         // 键盘加载类型
    std::string     mInputText{ "" };              // 输入框内容
    std::string     mDescription{ "" };            // 描述文本
    int             mMaxInputCount{ 10 };          // 最大输入长度
    OnCloseListener mCloseListener{ nullptr };     // 关闭回调

private:
    EditText*       mInputTextEdit{ nullptr };     // 输入框
    Button*         mCompleteBtn{ nullptr };       // 完成按钮
    Button*         mCancelBtn{ nullptr };         // 取消按钮
    ViewGroup*      mChildBox{ nullptr };          // 子键盘容器

    std::set<CKeyBoardChild*> mChilds;             // 子键盘
    std::vector<KeyBoardType> mEnableChilds;       // 启用的子键盘
};

#endif // __C_KEYBOARD_H__