/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2024-12-13 19:13:40
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

#include "base.h"

class BaseWindow :public Window, public EventHandler {
public:
protected:
    Context*          mContext;
    LayoutInflater*   mInflater;
    ViewGroup*        mBaseView;
    ViewGroup*        mMainBox;
    ViewGroup*        mPopBox;
    TextView*         mTimeText;
    ImageView*        mWifiView;

    uint64_t          m200msTick;
    uint64_t          m1sTick;
    uint64_t          m2sTick;

    bool              mIsShowLogo;  // 
    View*             mLogo;        // LOGO
    Runnable          mCloseLogo;   // LOGO计时

    PopBase*          mPop;         // 弹窗
    Runnable          mPopTextRun;  // 弹幕计时
    TextView*         mPopText;     // 弹幕
    PageBase*         mPage;        // 页面

    int8_t            mPopTextLevel;  // 弹幕文本等级
    bool              mPopTextRunning;// 弹幕文本是否正在显示

    bool              mIsBlackView;
    View*             mBlackView;
private:
    BaseWindow();
    ~BaseWindow();
public:
    static BaseWindow* ins() {
        static BaseWindow* instance = new BaseWindow();
        return instance;
    }

    int checkEvents() override;
    int handleEvents() override;

    void init();
    void showLogo(uint32_t time = 2000);
    ImageView* getWifiView();

    virtual bool onKeyUp(int keyCode, KeyEvent& evt) override;
    virtual bool onKeyDown(int keyCode, KeyEvent& evt) override;
    bool onKey(uint16_t keyCode, uint8_t status);

    PageBase* getPage();
    PopBase*  getPop();
    int8_t    getPageType();
    int8_t    getPopType();
    bool      showBlack(bool upload = true);
    void      hideBlack();
    void      showPage(PageBase* page);
    bool      showPop(PopBase* pop);
    void      showToast(std::string text, int8_t level, bool animate = true, bool lock = false);
    void      removePage();
    void      removePop();
    void      hideToast();

    void      hideAll();

private:
    void timeTextTick();
    void btnLightTick();
};

#endif
