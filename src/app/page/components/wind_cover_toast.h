/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-30 15:35:53
 * @LastEditTime: 2026-06-30 17:59:32
 * @FilePath: /kk_frame/src/app/page/components/wind_cover_toast.h
 * @Description: 全屏覆盖Toast
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_COVER_TOAST_H__
#define __WIND_COVER_TOAST_H__

#include "tick_mgr.h"

#include <view/view.h>
#include <widget/textview.h>

class WindCoverToast {
private:
    bool        mIsInit{ false };        // 是否初始化

    TextView*   mToast{ nullptr };       // 息屏组件
    View*       mToastBox{ nullptr };    // 息屏组件容器

    int                    mDuration{ 2500 };    // 弹幕显示时间(ms)
    TickMgr::ITickVariable mToastTicker;         // 弹幕心跳
    int64_t                mToastEndTime{ 0 };   // 是否正在显示

    int            mGaussRadius{ 10 };           // 模糊背景圆角
    uint64_t       mGaussColor{ 0x99000000 };    // 模糊背景颜色
public:
    WindCoverToast();
    virtual ~WindCoverToast();

    void showToast(std::string text);
    void hideToast();
    bool isToastShow() const;

protected:
    void init(ViewGroup* parent);
    void setToastDuration(int duration);
    void setToastGauss(int radius, uint64_t color);

private:
    bool checkInit();
    void onTick(int64_t now);
    void applyGauss();
};

#endif // !__WIND_COVER_TOAST_H__