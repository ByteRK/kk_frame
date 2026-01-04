/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2026-01-04 14:42:28
 * @FilePath: /kk_frame/src/app/page/core/base.cc
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "base.h"

#include <core/app.h>
#include <widget/imageview.h>

/*
 *************************************** 基类 ***************************************
**/

PBase::PBase(std::string resource) {
    mContext = &App::getInstance();;
    mLooper = Looper::getMainLooper();
    mInflater = LayoutInflater::from(mContext);
    mLastTick = 0;

    int64_t startTime = SystemClock::uptimeMillis();
    mRootView = (ViewGroup*)mInflater->inflate(resource, nullptr);
    LOGI("Load UI[%s] cost:%lldms", resource.c_str(), SystemClock::uptimeMillis() - startTime);
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
    mIsAttach = true;
    onAttach();
}

void PBase::callDetach() {
    mIsAttach = false;
    onDetach();
}

void PBase::callLoad(LoadMsgBase* loadMsg) {
    onLoad(loadMsg);
}

SaveMsgBase* PBase::callSaveState() {
    return onSaveState();
}

void PBase::callRestoreState(const SaveMsgBase* saveMsg) {
    onRestoreState(saveMsg);
}

void PBase::callMsg(const RunMsgBase* runMsg) {
    onMsg(runMsg);
}

void PBase::callMcu(uint8_t* data, uint8_t len) {
    onMcu(data, len);
}

bool PBase::callKey(uint16_t keyCode, uint8_t evt) {
    LOGV("callKey -> keyCode:%d evt:%d", keyCode, evt);
    return onKey(keyCode, evt);
}

void PBase::callLangChange(uint8_t lang) {
    mLang = lang;
    onLangChange();
}

void PBase::callCheckLight(uint8_t* left, uint8_t* right) {
    onCheckLight(left, right);
}

void PBase::onTick() {
}

void PBase::onAttach() {
}

void PBase::onDetach() {
}

void PBase::onLoad(LoadMsgBase* loadMsg) {
}

SaveMsgBase* PBase::onSaveState() {
    return nullptr;
}

void PBase::onRestoreState(const SaveMsgBase* saveMsg) {
}

void PBase::onMsg(const RunMsgBase* runMsg) {
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
    else v->setText(JsonUtils::to<std::string>(value, "null"));
}