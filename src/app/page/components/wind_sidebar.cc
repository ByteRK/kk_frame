/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-25 14:05:21
 * @LastEditTime: 2026-06-25 14:32:56
 * @FilePath: /kk_frame/src/app/page/components/wind_sidebar.cc
 * @Description: 侧边栏组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_sidebar.h"
#include "wind_mgr.h"
#include "time_utils.h"
#include <widget/imageview.h>

WindSidebar::WindSidebar() {
    mTicker.setTick(1000);
    mTicker.setCallBack(std::bind(&WindSidebar::onTick, this, std::placeholders::_1));
}

WindSidebar::~WindSidebar() {
    mTicker.stopTick();
}

void WindSidebar::init(ViewGroup* parent) {
    if (mIsInit) return;

    if (!(mSidebar = PBase::get(parent, AppRid::sidebar)))
        throw std::runtime_error("WindSidebar init failed");
    mSidebar->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mSidebar->setSoundEffectsEnabled(false);

    mTimeTextView = PBase::get<TextView>(mSidebar, AppRid::time);

    auto click = [](View&){};
    ImageView* img = nullptr;
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_1))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_2))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);
    (img = PBase::get<ImageView>(mSidebar, AppRid::btn_3))->setOnClickListener(click);
    img->getDrawable()->setFilterBitmap(true);

    mIsInit = true;
}

void WindSidebar::showSidebar() {
    if (!checkInit() || isSidebarShow()) return;
    mSidebar->setVisibility(View::VISIBLE);
    mTicker.startTick(-1);
}

void WindSidebar::hideSidebar() {
    if (!checkInit() || !isSidebarShow()) return;
    mSidebar->setVisibility(View::GONE);
    mTicker.stopTick();
}

bool WindSidebar::isSidebarShow() {
    return mSidebar->getVisibility() == View::VISIBLE;
}

bool WindSidebar::checkInit() {
    if (mIsInit) return true;
    LOGE("Sidebar uninit");
    return false;
}

void WindSidebar::onTick(int64_t nowMs) {
    mTimeTextView->setText(TimeUtils::getTimeStr());
}
