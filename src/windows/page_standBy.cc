/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2024-12-16 10:12:29
 * @FilePath: /kk_frame/src/windows/page_standBy.cc
 * @Description: 待机页面
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#include "page_standBy.h"
#include "manage.h"

#include "btn_mgr.h"
#include "global_data.h"
#include "tuya_mgr.h"

StandByPage::StandByPage() :PageBase("@layout/page_standby") {
    initUI();
}

StandByPage::~StandByPage() {
}

void StandByPage::onReload() {
    LOGI("StandByPage::reload");
    onTick();
}

void StandByPage::onTick() {
}

uint8_t StandByPage::getType() const {
    return PAGE_STANDBY;
}

void StandByPage::getView() {
}

void StandByPage::setAnim() {
}

void StandByPage::setView() {
}

void StandByPage::loadData() {
}

/// @brief 按键监听
/// @param keyCode 
/// @param status 
/// @return 是否响铃
bool StandByPage::onKey(uint16_t keyCode, uint8_t status) {
    return false;
}