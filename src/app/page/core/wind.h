/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-06-25 11:16:43
 * @FilePath: /kk_frame/src/app/page/core/wind.h
 * @Description: 主窗口类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_H__
#define __WIND_H__

#include "template/singleton.h"
#include "tick_mgr.h"

#include "wind_logo.h"
#include "wind_black.h"
#include "wind_screensave.h"
#include "wind_page.h"
#include "wind_pop.h"
#include "wind_toast.h"
#include "wind_keyboard.h"

#include <widget/cdwindow.h>

class MainWindow :public Singleton<MainWindow>, public Window,
    public WindLogo, public WindBlack, public WindScreenSave,
    public WindPage, public WindPop,
    public WindToast, public WindKeyboard {
    friend Singleton<MainWindow>;

public:
    uint64_t          mLastAction{ 0 };       // 上次用户动作时间

protected:
    Context*          mContext{ nullptr };    // 上下文
    ViewGroup*        mRootView{ nullptr };   // 根容器

protected:
    int64_t                mExitTime{ 0 };    // 退出时间
    int64_t                mRebootTime{ 0 };  // 重启时间
    TickMgr::ITickVariable mEndTicker;        // 结束心跳器

private:
    MainWindow();

public:
    ~MainWindow();

    void  init();
    void  hideAll();

    void  postExit(int64_t delay = 2000);
    void  postReboot(int64_t delay = 2000);
    bool  isAppWillEnd();

protected:
    WindLogo::LOGO_INFO
              getLogo() override;

    bool      dispatchTouchEvent(MotionEvent& evt) override;
    bool      dispatchKeyEvent(KeyEvent& evt) override;

private:
    bool      selfKey(KeyEvent& evt);
    void      endTick(int64_t now);
};

#endif // !__WIND_H__
