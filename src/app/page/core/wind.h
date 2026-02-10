/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-02-11 01:00:33
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

#include "wind_logo.h"
#include "wind_black.h"
#include "wind_page.h"
#include "wind_pop.h"
#include "wind_toast.h"
#include "wind_keyboard.h"

#include <widget/cdwindow.h>

class MainWindow :public Singleton<MainWindow>,
    public Window, public EventHandler,
    public WindLogo, public WindBlack,
    public WindPage, public WindPop,
    public WindToast, public WindKeyboard {
    friend Singleton<MainWindow>;

public:
    uint64_t          mLastAction;     // 上次用户动作时间

protected:
    Context*          mContext;        // 上下文
    ViewGroup*        mRootView;       // 根容器

private:
    MainWindow();

public:
    ~MainWindow();

    void  init();
    void  hideAll();

protected:
    WindLogo::LOGO_INFO
              getLogo() override;
    
    int       checkEvents() override;
    int       handleEvents() override;
    bool      dispatchTouchEvent(MotionEvent& evt) override;
    bool      onKeyUp(int keyCode, KeyEvent& evt) override;
    bool      onKeyDown(int keyCode, KeyEvent& evt) override;

private:
    bool      onKey(int keyCode, KeyEvent& evt);
    bool      selfKey(int keyCode, KeyEvent& evt, bool& result);
};

#endif // !__WIND_H__
