/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2026-06-30 15:23:19
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
    mContext = &App::getInstance();
    mLooper = Looper::getMainLooper();
    mInflater = LayoutInflater::from(mContext);

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

void PBase::callAttach() {
    mIsAttach = true;
    onAttach();
}

void PBase::callDetach() {
    mIsAttach = false;
    onDetach();
}

void PBase::callLoad(const LoadBase* loadMsg) {
    onLoad(loadMsg);
}

std::unique_ptr<SaveBase> PBase::callSave() {
    return std::unique_ptr<SaveBase>(onSave());
}

void PBase::callRestore(const SaveBase* saveMsg) {
    onRestore(saveMsg);
}

bool PBase::callKey(KeyEvent& evt) {
    LOGV("callKey -> keyCode:%d evt:%d", evt.getKeyCode(), evt.getAction());
    return onKey(evt);
}

void PBase::callLangChange(uint8_t lang) {
    mLang = lang;
    onLangChange();
}

bool PBase::canAutoRecycle() const {
    return true;
}

void PBase::sendPrivateMsg(int32_t what, void * data) {
    LOGI("sendPrivateMsg -> what:%d", what);
}

void PBase::onAttach() { }

void PBase::onDetach() { }

void PBase::onLoad(const LoadBase* loadMsg) { }

SaveBase* PBase::onSave() {
    return nullptr;
}

void PBase::onRestore(const SaveBase* saveMsg) { }

void PBase::onTick(int64_t nowMs) { }

bool PBase::onKey(KeyEvent& evt) {
    return false;
}

void PBase::onLangChange() { }

void PBase::setLangText(TextView* v, const Json::Value& value) {
    if (v == nullptr) LOGE("TextView is nullptr");
    else v->setText(JsonUtils::to<std::string>(value, "null"));
}
