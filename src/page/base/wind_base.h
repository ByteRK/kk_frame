/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2025-11-25 17:46:15
 * @FilePath: /kk_frame/src/windows/wind_base.h
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef _WIND_BASE_H_
#define _WIND_BASE_H_

#include <widget/cdwindow.h>
#include <widget/textview.h>
#include <widget/imageview.h>
#include "video_view.h"

#include "base.h"

class BaseWindow :public Window, public EventHandler {
    typedef enum {
        LOGO_TYPE_IMG = 0,   // 图片LOGO
        LOGO_TYPE_ANI,       // 动画LOGO
        LOGO_TYPE_VIDEO,     // 视频LOGO
    } LOGO_TYPE;

    typedef struct {
        std::string text = "";
        int8_t      level = 0;
        bool        animate = true;
        bool        lock = false;
    } TOAST_TYPE;
public:
    uint64_t          mLastAction;     // 上次用户动作时间
protected:
    Context*          mContext;        // 上下文
    ViewGroup*        mRootView;       // 根容器
    ViewGroup*        mPageBox;        // 页面容器
    ViewGroup*        mPopBox;         // 弹窗容器
    ImageView*        mLogoForImage;   // 图片LOGO
    VideoView*        mLogoForVideo;   // 视频LOGO
    TextView*         mToast;          // 弹幕
    View*             mBlackView;      // 黑屏

    PopBase*          mPop;            // 弹窗指针
    PageBase*         mPage;           // 页面指针

    uint64_t          mLastTick;       // 上次Tick时间
    uint64_t          mPopTickTime;    // 弹窗Tick时间
    uint64_t          mPageTickTime;   // 页面Tick时间
    
    Runnable          mCloseLogo;      // LOGO计时
    bool              mIsShowLogo;     // LOGO是否正在显示
    Animatable2::AnimationCallback mLogoForImageCallback;    // LOGO动画回调

    bool              mIsBlackView;    // 黑屏是否正在显示

    Runnable          mToastRun;       // 弹幕计时
    int8_t            mToastLevel;     // 弹幕文本等级
    bool              mToastRunning;   // Toast是否正在显示
    std::queue<TOAST_TYPE> mToastList; // 弹幕队列
private:
    BaseWindow();
public:
    ~BaseWindow();
    static BaseWindow* ins() {
        static BaseWindow* instance = new BaseWindow();
        return instance;
    }

    int checkEvents() override;
    int handleEvents() override;

    void init();
    void showLogo(uint32_t time = 2000);
    virtual LOGO_TYPE getLogo(std::string& path);

    bool onKeyUp(int keyCode, KeyEvent& evt) override;
    bool onKeyDown(int keyCode, KeyEvent& evt) override;
    bool onKey(uint16_t keyCode, uint8_t status);
    bool dispatchTouchEvent(MotionEvent& evt) override;

    PageBase* getPage();
    PopBase*  getPop();
    int8_t    getPageType();
    int8_t    getPopType();
    bool      showBlack(bool upload = true);
    void      hideBlack();
    int8_t    showPage(PageBase* page, LoadMsgBase* initData = nullptr);
    int8_t    showPop(PopBase* pop, LoadMsgBase* initData = nullptr);
    void      showToast(std::string text, int8_t level, bool keepNow = false, bool animate = true, bool lock = false);
    void      removePage();
    void      removePop();
    void      hideToast();
    void      hideAll();
private:
    bool      selfKey(uint16_t keyCode, uint8_t status);
    
    void      toastTick();
    void      btnLightTick();
};

#endif
