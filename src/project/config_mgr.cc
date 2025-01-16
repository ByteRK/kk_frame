/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-01-17 01:25:47
 * @FilePath: /kk_frame/src/project/config_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include "config_mgr.h"
#include "defualt_config.h"

#include "version.h"
#include "comm_func.h"
#include "global_data.h"

#include <cdlog.h>
#include <unistd.h>

#include "proto.h"
#include "manage.h"

#define TIME_INTERVAL (1000 * 10) // 备份时间间隔 10s

configMgr::configMgr(/* args */) {
}

configMgr::~configMgr() {
    mLooper->removeMessages(this);
}

/// @brief 保存配置文件
void configMgr::update() {
    uint64_t now = SystemClock::uptimeMillis();
    if (mConfig.getUpdates()) {
        mUpdates += mConfig.getUpdates();
        mConfig.save(CONFIG_FILE_PATH);
        sync();
        mMsg.when = now + TIME_INTERVAL;
        LOG(INFO) << "[config] save config. file=" << CONFIG_FILE_PATH;
    }
    if (mUpdates && (now >= mMsg.when)) {
        mConfig.save(CONFIG_FILE_BAK_PATH);
        sync();
        mUpdates = 0;
        LOG(INFO) << "[config] save config. file=" << CONFIG_FILE_BAK_PATH;
    }
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void configMgr::handleMessage(Message& message) {
    update();
    mLooper->sendMessageDelayed(1000, this, mMsg);
}

/// @brief 初始化
void configMgr::init() {
    LOG(INFO) << "[config] load config. file=" << CONFIG_FILE_PATH;

    if (access(CONFIG_FILE_PATH, F_OK) == 0) {
        mConfig.load(CONFIG_FILE_PATH);
    } else if (access(CONFIG_FILE_BAK_PATH, F_OK) == 0) {
        mConfig.load(CONFIG_FILE_BAK_PATH);
    } else {
        mConfig.setValue(CONFIG_SECTION, HV_STRING(MCU), HV_SYS_CONFIG_MCU);
    }

    mUpdates = 0;
    mNextBakTick = SystemClock::uptimeMillis();

    mMsg.what = 1;
    mMsg.when = mNextBakTick;
    mLooper = Looper::getMainLooper();
    mLooper->sendMessageDelayed(2000, this, mMsg);
}

/**************************************************************************************/
