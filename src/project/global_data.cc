/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-02-18 19:22:25
 * @FilePath: /kk_frame/src/project/global_data.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include "global_data.h"
#include "this_func.h"
#include "comm_func.h"
#include "json_func.h"
#include "base_data.h"

#include "defualt_config.h"

#include <unistd.h>

// 备份时间间隔
constexpr uint32_t TIME_INTERVAL = 1000 * 10;

 /// @brief 
globalData::globalData() {
}

/// @brief 
globalData::~globalData() {
    mLooper->removeMessages(this);
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void globalData::handleMessage(Message& message) {
    uint64_t now = SystemClock::uptimeMillis();
    switch (message.what) {
    case MSG_SAVE:
        if (mHaveChange) {
            saveToFile();
            sync();
            mHaveChange = false;
            mNeedSaveBak = true;
            mSaveMsg.when = now + TIME_INTERVAL;
            LOG(INFO) << "[app] save globalData. file=" << APP_FILE_FULL_PATH;
        }
        if (mNeedSaveBak && (now >= mSaveMsg.when)) {
            saveToFile(true);
            sync();
            mNeedSaveBak = false;
            LOG(INFO) << "[app] save globalData bak. file=" << APP_FILE_BAK_PATH;
        }
        mLooper->sendMessageDelayed(2000, this, mSaveMsg);
        break;
    default:
        break;
    }
}

/// @brief 初始化
void globalData::init() {
    mPowerOnTime = SystemClock::uptimeMillis();
    loadFromFile();

    mHaveChange = false;
    mNeedSaveBak = false;
    mNextBakTick = SystemClock::uptimeMillis();
    mSaveMsg.what = MSG_SAVE;
    mSaveMsg.when = mNextBakTick;
    mLooper = Looper::getMainLooper();

    mLooper->sendMessageDelayed(2000, this, mSaveMsg);
}

/// @brief 获取程序启动时间
/// @return 
uint64_t globalData::getPowerOnTime() {
    return mPowerOnTime;
}

/// @brief 载入本地文件
/// @return 
bool globalData::loadFromFile() {
    Json::Value appJson;
    std::string loadingPath = "";
    if (access(APP_FILE_FULL_PATH, F_OK) == 0) {
        loadingPath = APP_FILE_FULL_PATH;
    } else if (access(APP_FILE_BAK_PATH, F_OK) == 0) {
        loadingPath = APP_FILE_BAK_PATH;
    }
    LOG(INFO) << "Loading local data, file=" << loadingPath;
    if (!loadLocalJson(loadingPath, appJson))
        return false;

    return true;
}

/// @brief 保存文件到本地
/// @param isBak 
/// @return 
bool globalData::saveToFile(bool isBak) {
    Json::Value appJson(Json::arrayValue);
    if (isBak)
        return saveLocalJson(APP_FILE_BAK_PATH, appJson);
    else
        return saveLocalJson(APP_FILE_FULL_PATH, appJson);
    return true;
}