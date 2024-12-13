/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2024-12-13 18:59:12
 * @FilePath: /kk_frame/src/windows/base.cc
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */
#include <core/app.h>

#include "base.h"
#include "manage.h"

#include "this_func.h"
#include "conn_mgr.h"
#include "btn_mgr.h"


 /*
  *************************************** 基类 ***************************************
  */

PBase::PBase() {
    mContext = &App::getInstance();;
    mLooper = Looper::getMainLooper();
    mInflater = LayoutInflater::from(mContext);
    mLastTick = SystemClock::uptimeMillis();
    mLastClick = mLastTick;
}

PBase::~PBase() {
    __delete(mRootView);
}

uint8_t PBase::getLang() const {
    return mLang;
}

View* PBase::getRootView() {
    return mRootView;
}

void PBase::callTick() {
    onTick();
}

void PBase::callAttach() {
    onReload();
}

void PBase::callDetach() {
    onDetach();
}

void PBase::callMcu(uint8_t* data, uint8_t len) {
    onMcu(data, len);
}

bool PBase::callKey(uint16_t keyCode, uint8_t evt) {
    LOGV("onKeyDown keyCode:%d", keyCode);
    mLastClick = SystemClock::uptimeMillis();
    if (keyCode == KEY_MUTE)return false;  // 刷新时间用
    return onKey(keyCode, evt);
}

void PBase::callLangChange(uint8_t lang) {
    mLang = lang;
    onLangChange();
}

void PBase::callCheckLight(uint8_t* left, uint8_t* right) {
    onCheckLight(left, right);
}

View* PBase::findViewById(int id) {
    if (!mRootView)return nullptr;
    return mRootView->findViewById(id);
}

void PBase::onTick() {
}

void PBase::onReload() {
}

void PBase::onDetach() {
}

void PBase::onMcu(uint8_t* data, uint8_t len) {
}

bool PBase::onKey(uint16_t keyCode, uint8_t evt) {
    return false;
}

void PBase::onLangChange() {
}

void PBase::onCheckLight(uint8_t* left, uint8_t* right) {
}

void PBase::setLangText(TextView* v, const Json::Value& value) {
    if (v == nullptr) LOGE("TextView is nullptr");
    else v->setText(jsonToString(value, "null"));
}

/*
 *************************************** 弹窗 ***************************************
 */

PopBase::PopBase(std::string resource) :PBase() {
    mRootView = (ViewGroup*)mInflater->inflate(resource, nullptr);
}

PopBase::~PopBase() {
}

/*
 *************************************** 页面 ***************************************
 */

 /// @brief 
 /// @param resource 
PageBase::PageBase(std::string resource) :PBase() {
    int64_t startTime = SystemClock::uptimeMillis();
    mRootView = (ViewGroup*)mInflater->inflate(resource, nullptr);
    LOGI("Load UI cost:%lldms", SystemClock::uptimeMillis() - startTime);

    g_windMgr->add(this);
}

/// @brief 析构
PageBase::~PageBase() {
}

/// @brief 初始化UI
void PageBase::initUI() {
    mInitUIFinish = false;
    getView();
    setAnim();
    setView();
    loadData();
    mInitUIFinish = true;
}