/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:22:41
 * @LastEditTime: 2025-12-31 11:18:48
 * @FilePath: /kk_frame/src/app/page/components/wind_toast.h
 * @Description: Toast组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_TOAST_H__
#define __WIND_TOAST_H__

#include <view/view.h>
#include <view/viewgroup.h>
#include <widget/textview.h>

class WindToast {
public:
    DECLARE_UIEVENT(void, OnShowToastListener, void); // 弹幕显示回调
private:
    typedef struct {
        std::string text = "";            // 弹幕文本
        int8_t      level = 0;            // 弹幕文本等级
        bool        animate = true;       // 是否动画显示
        bool        lock = false;         // 是否锁定
    } TOAST_TYPE;
private:
    bool              mIsInit;            // 是否初始化
    bool              mIsRunning;         // 是否正在显示
    Runnable          mRuner;             // 弹幕计时
    int8_t            mLevel;             // 弹幕文本等级
    std::queue<TOAST_TYPE> mList;         // 弹幕队列
    OnShowToastListener    mShowListener; // 弹幕显示回调

    TextView*         mToast;             // 弹幕指针
    ViewGroup*        mToastBox;          // 弹幕容器
    
    int               mDuration = 2500;   // 弹幕显示时间(ms)
    int               mAnimatime = 600;   // 弹幕动画时间(ms)
public:
    WindToast();
    ~WindToast();
    void init(ViewGroup* parent);
    void onTick();
    void showToast(std::string text);
    void showToast(std::string text, int8_t level, bool keepNow = false, bool animate = true, bool lock = false);
    void hideToast();
    bool isToastShow() const;

    void setToastDuration(int duration);
    void setToastAnimatime(int animatime);
    void setOnShowToastListener(OnShowToastListener listener);

private:
    inline bool checkInit();
};

#endif // __WIND_TOAST_H__
