/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-11-27 11:43:55
 * @FilePath: /cy_frame/src/project/config_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#include "config_mgr.h"
#include "config_info.h"
#include "comm_func.h"

#include <cdlog.h>
#include <unistd.h>

static constexpr uint32_t CM_SAVE_CHECK_INITERVAL = 2000;       // 检查保存间隔[2s]
static constexpr uint32_t CM_SAVE_BACKUP_INTERVAL = 1000 * 10;  // 备份间隔[10s]

configMgr::~configMgr() {
    mLooper->removeMessages(this);
}

/// @brief 初始化
void configMgr::init() {
    loadFromFile();

    mNextBakTime = UINT64_MAX;
    mCheckSaveMsg.what = MSG_SAVE;
    mLooper = Looper::getMainLooper();
    mLooper->sendMessageDelayed(CM_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
}

/// @brief 重置
void configMgr::reset() {
    std::string command = std::string("rm")\
        + " " + CONFIG_FILE_PATH + " " + CONFIG_FILE_BAK_PATH;
    std::system(command.c_str());
    init();
    LOGE("config_mgr factory reset.");
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void configMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_SAVE:
        checkToSave();
        break;
    default:
        break;
    }
}

/// @brief 从文件中加载配置
/// @return 
bool configMgr::loadFromFile() {
    mConfig = cdroid::Preferences(); // 清空原配置

    std::string loadingPath = "";
    if (checkFileExitAndNoEmpty(CONFIG_FILE_PATH)) {
        loadingPath = CONFIG_FILE_PATH;
    } else if (checkFileExitAndNoEmpty(CONFIG_FILE_BAK_PATH)) {
        loadingPath = CONFIG_FILE_BAK_PATH;
    } else {
        LOG(ERROR) << "[config] no config file found. create new config file.";
        return false;
    }
    LOG(INFO) << "[config] load config. file=" << loadingPath;

    /**** 开始读取数据 ****/
    mConfig.load(loadingPath);
    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存配置文件
void configMgr::checkToSave() {
    uint64_t now = SystemClock::uptimeMillis();
    if (mConfig.getUpdates()) {
        mConfig.save(CONFIG_FILE_PATH);
#ifndef CDROID_X64
        sync();
#endif
        mNextBakTime = now + CM_SAVE_BACKUP_INTERVAL;
        LOG(INFO) << "[config] save config. file=" << CONFIG_FILE_PATH;
    }
    if (now >= mNextBakTime) {
        mConfig.save(CONFIG_FILE_BAK_PATH);
#ifndef CDROID_X64
        sync();
#endif
        mNextBakTime = UINT64_MAX;
        LOG(INFO) << "[config] save config bak. file=" << CONFIG_FILE_BAK_PATH;
    }
    mLooper->sendMessageDelayed(CM_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
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
    if (value == getBrightness()) return;
    mConfig.setValue(CONFIG_SECTION, "BRIGHTNESS", value);
}

/// @brief 获取音量
/// @return 
int configMgr::getVolume() {
    return mConfig.getInt(CONFIG_SECTION, "VOLUME", CONFIG_VOLUME);
}

/// @brief 设置音量
/// @param value 
void configMgr::setVolume(int value) {
    if (value == getVolume()) return;
    mConfig.setValue(CONFIG_SECTION, "VOLUME", value);
}

/// @brief 获取自动锁屏
/// @return 
bool configMgr::getAutoLock() {
    return mConfig.getBool(CONFIG_SECTION, "AUTOLOCK", CONFIG_AUTOLOCK);
}

/// @brief 设置自动锁屏
/// @param value 
void configMgr::setAutoLock(bool value) {
    if (value == getAutoLock()) return;
    mConfig.setValue(CONFIG_SECTION, "AUTOLOCK", value);
}
