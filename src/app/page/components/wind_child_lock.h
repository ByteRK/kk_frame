/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-08-11 14:12:08
 * @LastEditTime: 2026-08-11 16:42:58
 * @FilePath: /kk_frame/src/app/page/components/wind_child_lock.h
 * @Description: 童锁组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_CHILD_LOCK_H__
#define __WIND_CHILD_LOCK_H__

#include "tick_mgr.h"

#include <view/viewgroup.h>
#include <widget/imageview.h>
#include "arc_seekbar.h"

class WindChildLock {
    // 时间常量
    static constexpr int64_t kBoxAutoHideDelayMs   = 5000;   // 解锁盒无操作自动隐藏延时
    static constexpr int64_t kUnlockAnimDurationMs = 3000;   // 解锁动画时长
    static constexpr int64_t kTickIntervalMs       = 1000;   // 定时器间隔
    static constexpr int     kSeekBarMin           = 0;      // 进度条最小值
    static constexpr int     kSeekBarMax           = 100;    // 进度条最大值
    static constexpr int     kSeekBarPlus          = 5;      // 进度条增益(让进度条视觉上完整跑完)
    static constexpr int     kBoxActivateValue     = 90;     // 解锁盒容器激活值

private:
    bool          mIsInit{ false };            // 是否初始化
    View*         mRootView{ nullptr };        // 全屏触摸拦截层
    View*         mChildLockBox{ nullptr };    // 解锁盒容器（图标 + 进度条）
    View*         mChildLockCover{ nullptr };  // 童锁遮罩层
    ArcSeekBar*   mSeekBar{ nullptr };         // 弧形进度条

    bool            mUBF{ false };               // 首次触摸即开始解锁
    int64_t         mHideTime{ 0 };              // 自动隐藏时间戳（0 = 不自动隐藏）
    ValueAnimator*  mValueAnimator{ nullptr };   // 进度条动画
    TickMgr::ITickVariable mTickVariable;        // 定时器

    bool            mGaussEnable{ true };        // 是否启用模糊背景
    int             mGaussRadius{ 12 };          // 模糊背景圆角
    uint64_t        mGaussColor{ 0xDD000000 };   // 模糊背景颜色

public:
    WindChildLock();
    virtual ~WindChildLock();

    void openChildLock();
    void closeChildLock();
    bool isChildLockOpen() const;
    void setChildLockGauss(bool enable, int radius, uint64_t color);
    void setChildLockStartWithFirstTouch(bool ubf);

protected:
    void init(ViewGroup* parent);
    bool onKey(KeyEvent& evt);

private:
    bool checkInit();
    bool showChildLockBox();
    void hideChildLockBox();
    void startUnlockAnimation();
    void cancelUnlockAnimation();
    void onTick(int64_t now);
};

#endif // __WIND_CHILD_LOCK_H__
