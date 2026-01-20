/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-01-20 17:11:53
 * @FilePath: /kk_frame/src/app/page/core/wind.h
 * @Description: 主窗口类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_H__
#define __WIND_H__

#include "page.h"
#include "pop.h"
#include "template/singleton.h"
#include "wind_logo.h"
#include "wind_toast.h"

#include <widget/cdwindow.h>
#include <widget/textview.h>
#include <widget/imageview.h>

class MainWindow :public Singleton<MainWindow>,
    public Window, public EventHandler,
    public WindLogo, public WindToast {
    friend class Singleton<MainWindow>;
public:
    uint64_t          mLastAction;     // 上次用户动作时间
protected:
    Context*          mContext;        // 上下文
    ViewGroup*        mRootView;       // 根容器
    
    PopBase*          mPop;            // 弹窗指针
    ViewGroup*        mPopBox;         // 弹窗容器
    uint64_t          mPopNextTick;    // 弹窗下次Tick时间

    PageBase*         mPage;           // 页面指针
    ViewGroup*        mPageBox;        // 页面容器
    uint64_t          mPageNextTick;   // 页面下次Tick时间

    View*             mBlackView;      // 黑屏
    bool              mIsBlackView;    // 黑屏是否正在显示
private:
    MainWindow();

public:
    ~MainWindow();

    int checkEvents() override;
    int handleEvents() override;

    void init();
    virtual WindLogo::LOGO_INFO getLogo() override;

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
    void      removePage();
    void      removePop();
    void      hideAll();
private:
    bool      selfKey(uint16_t keyCode, uint8_t status);
};

#endif // !__WIND_H__
