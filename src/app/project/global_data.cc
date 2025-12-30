/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-12-30 15:47:50
 * @FilePath: /kk_frame/src/app/project/global_data.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "global_data.h"
#include "json_utils.h"
#include "file_utils.h"
#include "base_data.h"
#include "config_info.h"
#include <unistd.h>

static constexpr uint32_t GD_SAVE_CHECK_INITERVAL = 2000;       // 检查保存间隔[2s]
static constexpr uint32_t GD_SAVE_BACKUP_INTERVAL = 1000 * 10;  // 备份间隔[10s]

GlobalData::GlobalData() :mAppStart(SystemClock::uptimeMillis()) {
}

/// @brief 析构
GlobalData::~GlobalData() {
    mLooper->removeMessages(this);
}

/// @brief 初始化
void GlobalData::init() {
    mPowerOnTime = SystemClock::uptimeMillis();

    checkenv();
    loadFromFile();

    mNextBakTime = UINT64_MAX;
    mCheckSaveMsg.what = MSG_SAVE;
    mLooper = Looper::getMainLooper();
    mLooper->sendMessageDelayed(GD_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
}

/// @brief 重置
void GlobalData::reset() {
    std::string command = std::string("rm")\
        + " " + APP_FILE_FULL_PATH + " " + APP_FILE_BAK_PATH;
    std::system(command.c_str());
    init();
    LOGE("global_data factory reset.");
}

/// @brief 定时任务，用于保存修改后的配置
/// @param message 
void GlobalData::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_SAVE:
        checkToSave();
        break;
    default:
        break;
    }
}

/// @brief 检查环境变量
void GlobalData::checkenv() {
    // 设备模式
    if (getenv("DEV_MODE")) {
        uint8_t devMode = atoi(getenv("DEV_MODE"));
        if (devMode < DEVICE_MODE_MAX) {
            mDeviceMode = devMode;
        }
    }
    LOGW_IF(mDeviceMode, "DEVICE_MODE: %d", mDeviceMode);
}

/// @brief 载入本地文件
/// @return 
bool GlobalData::loadFromFile() {
    Json::Value appJson;
    std::string loadingPath = "";
    size_t fileLen = 0;

    if (FileUtils::check(APP_FILE_FULL_PATH, &fileLen) && fileLen > 0) {
        loadingPath = APP_FILE_FULL_PATH;
    } else if (FileUtils::check(APP_FILE_BAK_PATH, &fileLen) && fileLen > 0) {
        loadingPath = APP_FILE_BAK_PATH;
    }

    if (loadingPath.empty() || !JsonUtils::load(loadingPath, appJson)) {
        LOG(ERROR) << "[app] no local data file found. use default data";
        mHaveChange = true;
        return false;
    }
    LOG(INFO) << "[app] load local data. file=" << loadingPath;

    /**** 开始读取数据 ****/
    mCoffee = JsonUtils::get(appJson, "coffee", true);
    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存文件到本地
/// @param isBak 是否为备份
/// @return 
bool GlobalData::saveToFile(bool isBak) {
    Json::Value appJson;
    /**** 开始写入数据 ****/
    appJson["coffee"] = mCoffee;
    /**** 结束写入数据 ****/
    return JsonUtils::save(
        isBak ? APP_FILE_BAK_PATH : APP_FILE_FULL_PATH,
        appJson);
}

/// @brief 检查是否需要保存
void GlobalData::checkToSave() {
    uint64_t now = SystemClock::uptimeMillis();
    if (mHaveChange) {
        saveToFile();
        FileUtils::sync();
        mHaveChange = false;
        mNextBakTime = now + GD_SAVE_BACKUP_INTERVAL;
        LOG(INFO) << "[app] save GlobalData. file=" << APP_FILE_FULL_PATH;
    }
    if (now >= mNextBakTime) {
        saveToFile(true);
        FileUtils::sync();
        mNextBakTime = UINT64_MAX;
        LOG(INFO) << "[app] save GlobalData bak. file=" << APP_FILE_BAK_PATH;
    }
    mLooper->sendMessageDelayed(GD_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
}

/// @brief 获取程序启动时间[可粗略计算程序运行时间]
/// @return 
uint64_t GlobalData::getPowerOnTime() {
    return mPowerOnTime;
}