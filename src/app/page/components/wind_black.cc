/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:37:55
 * @LastEditTime: 2026-02-08 05:00:58
 * @FilePath: /kk_frame/src/app/page/components/wind_black.cc
 * @Description: 息屏组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_black.h"
#include "base.h"
#include "config_mgr.h"
#include "system_utils.h"

WindBlack::WindBlack() {
}

WindBlack::~WindBlack() {
}

/// @brief 初始化
/// @param parent 
void WindBlack::init(ViewGroup* parent) {
    if (mIsInit) return;

    if (!(mBlackView = PBase::get(parent, AppRid::black)))
        throw std::runtime_error("WindBlack init failed");

    // 获取节点
    mBlackView->setVisibility(View::GONE);
    mBlackView->setOnClickListener([this](View& view) { hideBlack(); });

    // 初始化亮度
    SystemUtils::setBrightness(g_config->getBrightness());

    mIsInit = true;
}

/// @brief 显示息屏
void WindBlack::showBlack() {
    if (!checkInit() || isBlackShow()) return;
    SystemUtils::setBrightness(0);
    mBlackView->setVisibility(View::VISIBLE);
}

/// @brief 关闭息屏
void WindBlack::hideBlack() {
    if (!checkInit() || !isBlackShow()) return;
    mBlackView->setVisibility(View::GONE);
    SystemUtils::setBrightness(g_config->getBrightness());
}

/// @brief 检查当前是否显示
/// @return 是否显示
bool WindBlack::isBlackShow() {
    return mBlackView->getVisibility() == View::VISIBLE;
}

/// @brief 按键监听
/// @param keyCode 键值
/// @param evt 事件
/// @param result 处理结果
/// @return 是否已消费 为true则下层不再处理
bool WindBlack::onKey(int keyCode, KeyEvent& evt, bool& result) {
    if (!isBlackShow())return false;
    if (evt.getAction() == KeyEvent::ACTION_UP) {
        hideBlack();
        result = true;
    }
    return true;
}

/// @brief 检查当前是否已初始化
/// @return 是否已初始化
inline bool WindBlack::checkInit() {
    if (mIsInit) return true;
    LOGE("Black uninit");
    return false;
}
