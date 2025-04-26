/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2025-02-18 19:43:26
 * @FilePath: /cy_frame/src/windows/wind_base.h
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#ifndef _WIND_BASE_H_
#define _WIND_BASE_H_

#include <widget/cdwindow.h>
#include <widget/textview.h>

#include "base.h"

class BaseWindow :public Window, public EventHandler {
public:
    uint64_t          mLastAction;     // 上次用户动作时间
protected:
    Context*          mContext;        // 上下文
    ViewGroup*        mRootView;       // 根容器
    ViewGroup*        mPageBox;        // 页面容器
    ViewGroup*        mPopBox;         // 弹窗容器
    View*             mLogo;           // LOGO
    TextView*         mToast;          // 弹幕
    View*             mBlackView;      // 黑屏

    PopBase*          mPop;            // 弹窗指针
    PageBase*         mPage;           // 页面指针

    uint64_t          mLastTick;       // 上次Tick时间
    uint64_t          mPopTickTime;    // 弹窗Tick时间
    uint64_t          mPageTickTime;   // 页面Tick时间
    
    bool              mIsShowLogo;     // LOGO是否正在显示
    bool              mIsBlackView;    // 黑屏是否正在显示
    bool              mToastRunning;   // Toast是否正在显示

    Runnable          mCloseLogo;      // LOGO计时
    Runnable          mToastRun;       // 弹幕计时
    int8_t            mToastLevel;     // 弹幕文本等级
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
    void      showPage(PageBase* page);
    void      showPop(PopBase* pop);
    void      showToast(std::string text, int8_t level, bool animate = true, bool lock = false);
    void      removePage();
    void      removePop();
    void      hideToast();
    void      hideAll();
private:
    bool      selfKey(uint16_t keyCode, uint8_t status);
    
    void      btnLightTick();
};

#endif
