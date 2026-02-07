/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:22:41
 * @LastEditTime: 2026-02-08 04:16:29
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

/**
 * 默认为Alpha动画，可以通过setDiyToastAni设置自定义动画
 * 另外如果自定义动画的时候，如果stop()依旧是动画的话，那么start必须调用cancel()取消动画
 * 
 * eg:
 * 
 * WindToast::TOAST_ANIMATE anim;
 * anim.start = [](View& v, int duration) {
 *     v.animate().cancel();
 *     v.setTranslationY(118);
 *     v.animate().translationY(0).setDuration(duration).start();
 * };
 * anim.finish = [](View& v, int duration) {
 *     v.animate().translationY(118).setDuration(duration).start();
 * };
 * anim.stop = [](View& v, int duration) {
 *     v.animate().cancel();
 *     v.animate().translationY(118).setDuration(duration).start();
 * };
 * anim.aniEnd = [](View& v, int duration) {
 *     // if (v.getTranslationY() == 118)v.setVisibility(View::GONE);
 * };
 * setDiyToastAni(anim);
 * 
**/

class WindToast {
public:
    DECLARE_UIEVENT(void, OnShowToastListener, void); // 弹幕显示回调

    DECLARE_UIEVENT(void, ToastAniListener, View&, int);
    struct TOAST_ANIMATE {
        ToastAniListener start;   // 开始动画
        ToastAniListener finish;  // 结束动画
        ToastAniListener stop;    // 停止动画(手动Hide Toast)

        ToastAniListener aniEnd;  // View回调，已注册到Toast.animate()
    };
    
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
    TOAST_ANIMATE     mDiyToastAni;       // 自定义弹幕动画
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
    void setDiyToastAni(TOAST_ANIMATE ani);

private:
    bool checkInit();

    void onStart(bool withAnim);
    void onFinish();
    void onStop();
    void onAnimEnd();
};

#endif // __WIND_TOAST_H__
