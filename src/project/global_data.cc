/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2024-08-22 18:10:09
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

#include "hv_defualt_config.h"

#include <unistd.h>

#define TIME_INTERVAL (1000 * 10)

#define SAVE_MODE_FILE false;

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
            saveMode();
            sync();
            mHaveChange = false;
            mNeedSaveBak = true;
            mSaveMsg.when = now + TIME_INTERVAL;
            LOG(INFO) << "[app] save globalData. file=" << APP_FILE_FULL_PATH;
        }
        if (mNeedSaveBak && (now >= mSaveMsg.when)) {
            saveMode(true);
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
    loadMode();

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

/// @brief 载入模式数据
void globalData::loadMode() {
    LOG(INFO) << "Loading built-in data";
}

/// @brief 载入本地文件
/// @return 
bool globalData::loadFromFile() {
    // Json::Value appJson;
    // std::string loadingPath = "";
    // if (access(APP_FILE_FULL_PATH, F_OK) == 0) {
    //     loadingPath = APP_FILE_FULL_PATH;
    // } else if (access(APP_FILE_BAK_PATH, F_OK) == 0) {
    //     loadingPath = APP_FILE_BAK_PATH;
    // }
    // if (!convertStringToJson(loadingPath, appJson) || !appJson.isArray())
    //     return false;
    return true;
}

/// @brief 保存文件到本地
/// @param isBak 
/// @return 
bool globalData::saveMode(bool isBak) {
    // Json::Value appJson(Json::arrayValue);
    // if (isBak)
    //     return saveLocalJson(APP_FILE_BAK_PATH, appJson);
    // else
    //     return saveLocalJson(APP_FILE_FULL_PATH, appJson);
    return true;
}