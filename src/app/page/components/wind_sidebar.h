/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-25 14:05:16
 * @LastEditTime: 2026-06-25 14:26:27
 * @FilePath: /kk_frame/src/app/page/components/wind_sidebar.h
 * @Description: 侧边栏组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_SIDEBAR_H__
#define __WIND_SIDEBAR_H__

#include "tick_mgr.h"

#include <view/view.h>
#include <widget/textview.h>

class WindSidebar {
private:
    View*   mSidebar{ nullptr };         // 侧边栏组件
    bool    mIsInit{ false };            // 是否初始化

    TickMgr::ITickVariable mTicker;      // 定时器

private:
    TextView* mTimeTextView{ nullptr };  // 屏保时间文本

public:
    WindSidebar();
    virtual ~WindSidebar();

    void init(ViewGroup* parent);
    void showSidebar();
    void hideSidebar();
    bool isSidebarShow();

private:
    bool checkInit();
    void onTick(int64_t nowMs);
};

#endif // !__WIND_SIDEBAR_H__
