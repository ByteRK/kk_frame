/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-30 15:35:53
 * @LastEditTime: 2026-06-30 18:02:37
 * @FilePath: /kk_frame/src/app/page/components/wind_cover_toast.cc
 * @Description: 全屏覆盖Toast
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_cover_toast.h"
#include "base.h"
#include "gauss_drawable.h"
#include "wind_mgr.h"

WindCoverToast::WindCoverToast() {
    mToastTicker.setTick(200);
    mToastTicker.setCallBack([this](int64_t now) { this->onTick(now); });
    mToastTicker.startTick(1000);
}

WindCoverToast::~WindCoverToast() {
    mToastTicker.stopTick();
}

/// @brief 显示Toast
/// @param text 
void WindCoverToast::showToast(std::string text) {
    if (!checkInit()) return;
    mToast->setText(text);
    if (!isToastShow()) {
        applyGauss();
        mToastBox->setVisibility(View::VISIBLE);
    }
    mToastEndTime = SystemClock::uptimeMillis() + mDuration;
}

/// @brief 隐藏Toast
void WindCoverToast::hideToast() {
    if (!checkInit()) return;
    mToastEndTime = 0;
    mToastBox->setVisibility(View::GONE);
}

/// @brief 获取Toast显示状态
/// @return 
bool WindCoverToast::isToastShow() const {
    return mToastEndTime != 0;
}

/// @brief 初始化
/// @param parent 
void WindCoverToast::init(ViewGroup* parent) {
    if (mIsInit) return;

    mToastBox = PBase::get(parent, AppRid::toast);
    FailFast(mToastBox == nullptr, "WindCoverToast init failed");

    // 获取节点
    mToast = PBase::get<TextView>(mToastBox, AppRid::toast_text);
    FailFast(mToast == nullptr, "WindCoverToast toast_text init failed");

    // 设置初始状态
    mToastBox->setVisibility(View::GONE);
    mToastBox->setOnClickListener([this](View& v) { this->hideToast(); });

    mIsInit = true;
}

/// @brief 设置Toast显示时间
/// @param duration 
void WindCoverToast::setToastDuration(int duration) {
    mDuration = duration;
}

/// @brief 设置Toast高斯模糊
/// @param radius 
/// @param color 
void WindCoverToast::setToastGauss(int radius, uint64_t color) {
    mGaussRadius = radius;
    mGaussColor = color;
}

/// @brief 检查是否初始化
/// @return 
bool WindCoverToast::checkInit() {
    return mIsInit;
}

/// @brief Tick事件
/// @param now 
void WindCoverToast::onTick(int64_t now) {
    if (!checkInit() || !isToastShow()) return;
    if (now >= mToastEndTime) hideToast();
}

/// @brief 应用高斯模糊
void WindCoverToast::applyGauss() {
#if ENABLED(GAUSS_DRAWABLE) || defined(__VSCODE__)
    mToastBox->setBackground(new GaussDrawable(g_window->getRegularLayer(), mGaussRadius, 0.5f, mGaussColor, true));
#else
    mToastBox->setBackgroundResource(mGaussColor);
    LOGD("WindCoverToast::applyGauss() gauss drawable not enabled");
#endif
}
