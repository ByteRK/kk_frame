/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2024-08-23 13:59:06
 * @FilePath: /kk_frame/src/windows/base.cc
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include "base.h"
#include "manage.h"

#include "this_func.h"
#include "conn_mgr.h"
#include "btn_mgr.h"

 /// @brief 
 /// @param resource 
PageBase::PageBase(std::string resource) :RelativeLayout(-1, -1) {
    mContext = getContext();
    mloop = Looper::getMainLooper();
    mInflater = LayoutInflater::from(getContext());

    int64_t startTime = SystemClock::uptimeMillis();
    mRootView = (ViewGroup*)mInflater->inflate(resource, this);
    LOGI("Load UI cost:%lldms", SystemClock::uptimeMillis() - startTime);

    mInitUIFinish = false;
    mLastClick = SystemClock::uptimeMillis();

    mGogoHomeRunner = [this] { this->goToHome(); };
    mGoToBackRunner = [this] { this->goToBack(); };
}

/// @brief 析构
PageBase::~PageBase() {
}

/// @brief 挂载刷新
void PageBase::reload() {
}

/// @brief 定时更新页面
void PageBase::onTick() {
}

/// @brief 按键信息预处理
/// @param keyCode 
/// @param status 
/// @return 是否响铃
bool PageBase::baseOnKey(uint16_t keyCode, uint8_t status) {
    LOGV("onKeyDown keyCode:%d", keyCode);
    mLastClick = SystemClock::uptimeMillis();
    if (keyCode == KEY_MUTE)return false;  // 刷新时间用
    return onKey(keyCode, status);
}

/// @brief 按键信息
/// @param keyCode 
/// @param status 
/// @return 是否响铃
bool PageBase::onKey(uint16_t keyCode, uint8_t status) {
    return false;
}

/// @brief 初始化UI
void PageBase::initUI() {
    mInitUIFinish = false;
    g_windMgr->add(this);
    getView();
    setAnim();
    setView();
    loadData();
    reload();
    mInitUIFinish = true;
}

/// @brief 获取wifiIcon指针
/// @return 
View* PageBase::getWifiView() {
    return BaseWindow::ins()->getWifiView();
}

/// @brief 返回上一页
/// @param delay 延时返回 默认立即返回
void PageBase::goToBack(uint32_t delay) {
}

/// @brief 返回主页
/// @param delay 延时返回 默认立即返回
void PageBase::goToHome(uint32_t delay) {
}

void PageBase::hideAll() {
    BaseWindow::ins()->hideAll();
}

/// @brief 获取弹窗
/// @return 
PopBase* PageBase::getPop() {
    return BaseWindow::ins()->getPop();
}

/// @brief 显示弹窗
/// @param type 
/// @return 
bool PageBase::showPop(int8_t type) {
    if (type == BaseWindow::ins()->getPopType())return true;
    if (type < BaseWindow::ins()->getPopType())return false;
#define BASE_SHOW_POP(t,c) case t: return BaseWindow::ins()->showPop(new c(mContext));
    switch (type) {
    default:
        return false;
    }
}

/// @brief 移除弹窗
void PageBase::removePop() {
    BaseWindow::ins()->removePop();
}

bool PageBase::showBlack(bool upload) {
    return BaseWindow::ins()->showBlack(upload);
}

void PageBase::removeBlack() {
    BaseWindow::ins()->hideBlack();
}

/// @brief 显示文字提示
/// @param text 
/// @param time 
void PageBase::showPopText(std::string text, int8_t level, bool animate, bool lock) {
    BaseWindow::ins()->showPopText(text, level, animate, lock);
}

/// @brief 移除弹幕
void PageBase::removePopText() {
    BaseWindow::ins()->removePopText();
}