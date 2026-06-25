/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:36:51
 * @LastEditTime: 2026-06-25 14:26:31
 * @FilePath: /kk_frame/src/app/page/components/wind_screensave.h
 * @Description: 屏保组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_SCREENSAVE_H__
#define __WIND_SCREENSAVE_H__

#include "tick_mgr.h"

#include <view/view.h>
#include <widget/textview.h>

class WindScreenSave {
private:
    View*   mScreenSave{ nullptr };      // 屏保组件
    bool    mIsInit{ false };            // 是否初始化

    int64_t mStartTime{ 0 };             // 开始显示的时间
    TickMgr::ITickVariable mTicker;      // 定时器

private:
    TextView* mTimeTextView{ nullptr };  // 屏保时间文本

public:
    WindScreenSave();
    virtual ~WindScreenSave();

    void init(ViewGroup* parent);
    void setScreenSave(bool enable);
    void showScreenSave();
    void hideScreenSave();
    bool isScreenSaveShow();
    bool onKey(KeyEvent& evt);

private:
    bool checkInit();
    void onTick(int64_t nowMs);

private:
    void updateScreenSave();
};

#endif // !__WIND_SCREENSAVE_H__
