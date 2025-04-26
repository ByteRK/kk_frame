/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-02-20 22:25:48
 * @FilePath: /cy_frame/src/project/config_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#include "config_mgr.h"
#include "defualt_config.h"

#include <cdlog.h>
#include <unistd.h>

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
        mConfig.save(CONFIG_FILE_PATH);
        sync();
        mNextBakTime = now + TIME_INTERVAL;
        LOG(INFO) << "[config] save config. file=" << CONFIG_FILE_PATH;
    }
    if (now >= mNextBakTime) {
        mConfig.save(CONFIG_FILE_BAK_PATH);
        sync();
        mNextBakTime = 0xFFFFFFFF;
        LOG(INFO) << "[config] save config. file=" << CONFIG_FILE_BAK_PATH;
    }
    mLooper->sendMessageDelayed(2000, this, mMsg);
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void configMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_SAVE:
        update();
        break;
    default:
        break;
    }
}

/// @brief 初始化
void configMgr::init() {
    LOG(INFO) << "[config] load config. file=" << CONFIG_FILE_PATH;

    if (access(CONFIG_FILE_PATH, F_OK) == 0) {
        mConfig.load(CONFIG_FILE_PATH);
    } else if (access(CONFIG_FILE_BAK_PATH, F_OK) == 0) {
        mConfig.load(CONFIG_FILE_BAK_PATH);
    }

    mNextBakTime = 0xFFFFFFFF;
    mMsg.what = MSG_SAVE;
    mLooper = Looper::getMainLooper();
    mLooper->sendMessageDelayed(2000, this, mMsg);
}
/**************************************************************************************/

/// @brief 获取屏幕亮度
/// @return 
int configMgr::getBrightness() {
    return mConfig.getInt(CONFIG_SECTION, "BRIGHTNESS", CONFIG_BRIGHTNESS);
}

/// @brief 设置屏幕亮度
/// @param value 
void configMgr::setBrightness(int value) {
    mConfig.setValue(CONFIG_SECTION, "BRIGHTNESS", value);
}
