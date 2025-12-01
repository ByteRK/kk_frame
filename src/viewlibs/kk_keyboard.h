/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-28 11:24:16
 * @LastEditTime: 2025-12-01 19:24:16
 * @FilePath: /kk_frame/src/viewlibs/kk_keyboard.h
 * @Description: Ricken的自定义键盘
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef KEYBOARD_DISABLE // CMakeLists.txt -> add_definitions(-DKEYBOARD_ENABLE)

#ifndef _KK_KEYBOARD_H_
#define _KK_KEYBOARD_H_

#include "common.h"
#include <widget/button.h>
#include <widget/edittext.h>
#include <widget/relativelayout.h>

class KKKeyBoard : public RelativeLayout {
public:
    DECLARE_UIEVENT(void, OnCompleteListener, const std::string& editTxt);
    DECLARE_UIEVENT(void, OnCancelListener, void);

    /// @brief 键盘类型
    typedef enum {
        KB_TYPE_EN = 0,             // 英文
        KB_TYPE_CN,                 // 中文

        KB_TYPE_MAX,                // 最大值
    } KBTYPE;

private:
    /// @brief 特殊键位置定义
    typedef enum {
        KB_SPECIAL_POS_UPPER = 0,   // 大写
        KB_SPECIAL_POS_LOWER,       // 小写
        KB_SPECIAL_POS_NUMBER,      // 数字
        KB_SPECIAL_POS_BACK,        // 返回
        KB_SPECIAL_POS_MORE,        // 更多
        KB_SPECIAL_POS_LESS,        // 更少
        KB_SPECIAL_POS_SPACE_L,     // 空格左边
        KB_SPECIAL_POS_SPACE,       // 空格
        KB_SPECIAL_POS_SPACE_R,     // 空格右边
        KB_SPECIAL_POS_ENTER,       // 回车
    } KBSPECIAL_POS;

    /// @brief 键盘配置
    struct KeyboardConfig {
        KBTYPE      type;                                            // 类型
        std::string name;                                            // 名称
        bool        has_shift;                                       // 是否有shift键
        std::array<std::vector<std::string>, 3> normal_rows;         // 常规
        std::array<std::vector<std::string>, 3> shift_rows;          // 大写
        std::array<std::vector<std::string>, 3> number_rows;         // 数字
        std::array<std::vector<std::string>, 3> more_rows;           // 更多
        std::vector<std::string>                special_key;         // 特殊键
    };

private:
    std::array<KeyboardConfig, 3> mKBDatabase;                       // 键盘数据表
    std::vector<Button*>          mKeyView[4];                       // 按键指针
    EditText*                     mEnterTextView;                    // 输入框指针
    Button*                       mEnterButtonView;                  // 确认键指针
    Button*                       mCancelButtonView;                 // 取消键指针

    std::vector<TextView*>        mCandidateWordView[10];            // 候选词指针
    TextView*                     mPingYinView;                      // 拼音指针
    View*                         mPrePageView;                      // 上一页指针
    View*                         mNextPageView;                     // 下一页指针

    std::string                   mEnterText;                        // 输入框内容
    std::string                   mDescTex;                          // 输入框描述文本

    KBTYPE                        mKBType;                           // 当前键盘类型
    bool                          mIsShift;                          // 是否大写
    EditText::INPUTTYPE           mEditTextType;                     // 输入框类型
    int                           mWordMaxCount;                     // 输入字长度上限 (0为不限制)

    std::string                   mLastTxt;                          // 上一次的内容
    std::vector<std::string>      mHzList;                           // 汉字选择项
    int                           mHzPos;                            // 当前尾部位置
    int                           mHzCount;                          // 当前显示的可选词数量

    OnCompleteListener            mCompleteListener;                 // 确认键监听
    OnCancelListener              mCancelListener;                   // 取消键监听
    View::OnLayoutChangeListener  mOnCandidateLayoutChange;          // 候选词布局变化监听

public:
    KKKeyBoard(int w, int h);                                                // 手动构造
    KKKeyBoard(Context* ctx, const AttributeSet& attr);                      // XML构造
    ~KKKeyBoard() override;                                                  // 析构

    void show();                                                             // 显示
    void hide();                                                             // 隐藏

    void setType(KBTYPE type);                                               // 设置显示类型
    void setEditType(EditText::INPUTTYPE editType = EditText::TYPE_TEXT);    // 设置输入框类型
    void setWordMaxCount(int count);                                         // 设置输入字最大长度

    void setDescText(const std::string& txt);                                // 设置输入框描述文本
    void setDefaultText(const std::string& txt);                             // 设置输入框默认文本

    virtual void setOnCompleteListener(OnCompleteListener l);                // 设置完成监听
    virtual void setOnCancelListener(OnCancelListener l);                    // 设置取消监听

    std::string getEnterText();                                              // 获取输入框内容
private:
    void initKBDatebase();                                                   // 初始化键盘数据表


private:
    void setText(const std::string& txt);                                    // 修改输入框文本
    void setText(const std::string& txt, TextView* txtView,
                int maxLen = 40, int minSize = 16, int maxSize = 34);        // 修改指定控件文本

    void delLastCharacter(TextView* txtView);                                // 删除最后一个字符



};

#endif /* _KK_KEYBOARD_H_ */

#endif /* KEYBOARD_ENABLE */