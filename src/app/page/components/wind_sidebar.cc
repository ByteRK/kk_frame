/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-25 14:05:21
 * @LastEditTime: 2026-09-10 17:16:27
 * @FilePath: /kk_frame/src/app/page/components/wind_sidebar.cc
 * @Description: 侧边栏组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_sidebar.h"
#include "wind_mgr.h"
#include <widget/imageview.h>

#include "time_update.h"

WindSidebar::WindSidebar() { }

WindSidebar::~WindSidebar() {
    if (mTimeTextView && TimeUpdate::instance()->contains(mTimeTextView)) {
        TimeUpdate::instance()->remove(mTimeTextView);
    }
}

/// @brief 显示侧边栏
void WindSidebar::showSidebar() {
    if (!checkInit() || isSidebarShow()) return;
    mSidebar->setVisibility(View::VISIBLE);

    TimeUpdate::instance()->add(mTimeTextView);
}

/// @brief 隐藏侧边栏
void WindSidebar::hideSidebar() {
    if (!checkInit() || !isSidebarShow()) return;
    mSidebar->setVisibility(View::GONE);

    TimeUpdate::instance()->remove(mTimeTextView);
}

/// @brief 侧边栏是否显示
/// @return 
bool WindSidebar::isSidebarShow() const {
    return mSidebar->getVisibility() == View::VISIBLE;
}

/// @brief 暂存当前侧边栏开关状态
void WindSidebar::storeSidebarState() {
    if (!checkInit()) return;
    mStoredShowState = isSidebarShow();
    mHasStoredState = true;
}

/// @brief 恢复暂存的侧边栏开关状态
void WindSidebar::restoreSidebarState() {
    if (!checkInit() || !mHasStoredState) return;
    if (mStoredShowState) showSidebar();
    else hideSidebar();
}

/// @brief 初始化
/// @param parent 
void WindSidebar::init(ViewGroup* parent) {
    if (mIsInit) return;

    mSidebar = PBase::get(parent, AppRid::sidebar);
    FailFast(mSidebar == nullptr, "WindSidebar init failed");

    mSidebar->setVisibility(View::GONE);

    mSidebar->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mSidebar->setSoundEffectsEnabled(false);

    mTimeTextView = PBase::get<TextView>(mSidebar, AppRid::time);

    auto click = [](View&) { };
    ImageView* img = nullptr;
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_1))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_2))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_3))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);

    mIsInit = true;
}

/// @brief 检查是否初始化
/// @return 
bool WindSidebar::checkInit() {
    if (mIsInit) return true;
    LOGE("Sidebar uninit");
    return false;
}
