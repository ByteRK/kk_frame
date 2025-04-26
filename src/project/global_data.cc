/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-02-20 22:26:29
 * @FilePath: /cy_frame/src/project/global_data.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
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

/// @brief 保存数据文件
void globalData::update() {
    uint64_t now = SystemClock::uptimeMillis();
    if (mHaveChange) {
        saveToFile();
        sync();
        mHaveChange = false;
        mNextBakTime = now + TIME_INTERVAL;
        LOG(INFO) << "[app] save globalData. file=" << APP_FILE_FULL_PATH;
    }
    if (now >= mNextBakTime) {
        saveToFile(true);
        sync();
        mNextBakTime = 0xFFFFFFFF;
        LOG(INFO) << "[app] save globalData bak. file=" << APP_FILE_BAK_PATH;
    }
    mLooper->sendMessageDelayed(2000, this, mSaveMsg);
}

/// @brief 
globalData::~globalData() {
    mLooper->removeMessages(this);
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void globalData::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_SAVE:
        update();
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
    mNextBakTime = 0xFFFFFFFF;
    mSaveMsg.what = MSG_SAVE;
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