/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-25 10:32:08
 * @LastEditTime: 2026-06-30 00:16:25
 * @FilePath: /kk_frame/src/app/page/components/wind_screensave.cc
 * @Description: 屏保组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_screensave.h"
#include "wind_mgr.h"
#include "config_mgr.h"
#include "project_utils.h"
#include "time_utils.h"

WindScreenSave::WindScreenSave() {
    mTicker.setTick(1000);
    mTicker.setCallBack(std::bind(&WindScreenSave::onTick, this, std::placeholders::_1));
}

WindScreenSave::~WindScreenSave() {
    mTicker.stopTick();
}

/// @brief 启停屏保
/// @param enable 
void WindScreenSave::setScreenSave(bool enable) {
    enable ? mTicker.startTick() : mTicker.stopTick();
}

/// @brief 显示屏保
void WindScreenSave::showScreenSave() {
    if (!checkInit() || isScreenSaveShow()) return;
    mScreenSave->setVisibility(View::VISIBLE);
    mStartTime = SystemClock::uptimeMillis();
    updateScreenSave();
}

/// @brief 隐藏屏保
void WindScreenSave::hideScreenSave() {
    if (!checkInit() || !isScreenSaveShow()) return;
    mScreenSave->setVisibility(View::GONE);
}

/// @brief 是否在显示屏保
/// @return 
bool WindScreenSave::isScreenSaveShow() const {
    return mScreenSave->getVisibility() == View::VISIBLE;
}

/// @brief 初始化
/// @param parent 
void WindScreenSave::init(ViewGroup* parent) {
    if (mIsInit) return;

    mScreenSave = PBase::get(parent, AppRid::screensave);
    FailFast(mScreenSave == nullptr, "WindScreenSave init failed");

    mScreenSave->setVisibility(View::GONE);

    mScreenSave->setOnClickListener([this](View& view) { hideScreenSave(); });

    mTimeTextView = PBase::get<TextView>(mScreenSave, AppRid::time);

    mTicker.startTick(2000);

    mIsInit = true;
}

/// @brief 处理按键事件
bool WindScreenSave::onKey(KeyEvent& evt) {
    if (!isScreenSaveShow())return false;
    if (evt.getAction() == KeyEvent::ACTION_UP)
        hideScreenSave();
    return true;
}

/// @brief 检查是否初始化
/// @return 
bool WindScreenSave::checkInit() {
    if (mIsInit) return true;
    LOGE("ScreenSave uninit");
    return false;
}

/// @brief 定时器回调
/// @param nowMs 
void WindScreenSave::onTick(int64_t nowMs) {
    int64_t     diff(0);
    const char* toWhat(nullptr);
    if (isScreenSaveShow()) {
        diff = nowMs - mStartTime;
#ifdef PRODUCT_X64
        diff *= 4;
#endif
        toWhat = "black";
        if (diff >= 120 * 1000)
            g_window->showBlack();

        updateScreenSave();
    } else {
        diff = nowMs - g_window->mLastAction;
#ifdef PRODUCT_X64
        diff *= 4;
#endif
        toWhat = "screenSave";
        if (diff >= g_config->screenSaveTime() * 1000)
            showScreenSave();
    }
    LOGV("WindScreenSave onTick diff:%.3llds -> %s", diff / 1000, toWhat);
}

/// @brief 更新内容
void WindScreenSave::updateScreenSave() {
    mTimeTextView->setText(TimeUtils::getTimeFmtStr("%Y-%m-%d %H:%M:%S"));
}
