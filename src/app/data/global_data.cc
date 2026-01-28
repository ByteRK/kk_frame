/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-01-29 02:31:19
 * @FilePath: /kk_frame/src/app/data/global_data.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "global_data.h"
#include "arg_utils.h"
#include "json_utils.h"
#include "file_utils.h"
#include "base_data.h"
#include "config_info.h"
#include <unistd.h>
#include <core/app.h>
#include <core/systemclock.h>
#include <cdlog.h>

GlobalData::GlobalData() :
    mAppStart(cdroid::SystemClock::uptimeMillis()),
    AutoSaveItem(2000, 10000) {
}

/// @brief 析构
GlobalData::~GlobalData() {
}

/// @brief 初始化
void GlobalData::init() {
    mIsFirstInit = FileUtils::check(APP_FIRST_INIT_TAG);
    mDeviceMode = (
        cdroid::App::getInstance().getName() == (std::string("kk") + std::string("_frame"))
        ) ?
        DEVICE_MODE_DEMO : DEVICE_MODE_SAMPLE;

    checkenv();

    if (ArgUtils::get().isDemo) // 模板演示模式
        mDeviceMode = DEVICE_MODE_DEMO;
    if ((mTestPage = ArgUtils::get().selectPage) != 0) // 测试模式（进入随意页面）
        mDeviceMode = DEVICE_MODE_TEST;

    load();

    AutoSaveItem::init();
}

/// @brief 重置
void GlobalData::reset() {
    std::string command = std::string("rm")  \
        + " " + APP_FILE_PATH + " " + APP_FILE_BAK_PATH;
    std::system(command.c_str());
    setFirstInit(true);
    // FileUtils::sync(); // 不需要Sync，上一步已Sync
    init();
    mHaveChange = true;
    LOGE("global_data factory reset.");
}

/// @brief 设置首次初始化标记
/// @param first true:首次初始化 false:非首次初始化
void GlobalData::setFirstInit(bool first) {
    std::string command;
    command += first ? "touch " : "rm ";
    command += APP_FIRST_INIT_TAG;
    std::system(command.c_str());
    mIsFirstInit = first;
    FileUtils::sync();
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
bool GlobalData::load() {
    Json::Value appJson;
    std::string loadingPath = "";
    size_t fileLen = 0;

    if (FileUtils::check(APP_FILE_PATH, &fileLen) && fileLen > 0) {
        loadingPath = APP_FILE_PATH;
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
/// @param isBackup 是否为备份
bool GlobalData::save(bool isBackup) {
    Json::Value appJson;
    /**** 开始写入数据 ****/
    appJson["coffee"] = mCoffee;
    /**** 结束写入数据 ****/
    mHaveChange = false;
    return JsonUtils::save(
        isBackup ? APP_FILE_BAK_PATH : APP_FILE_PATH,
        appJson);
}

/// @brief 检查是否存在变动，触发保存
/// @return 
bool GlobalData::haveChange() {
    return mHaveChange;
}

// /// @brief 更新咖啡[🎐测试用]
// /// @param coffee 
// void GlobalData::updateCoffee(bool coffee) {
//     mCoffee = coffee;
//     mHaveChange = true; // 触发自动保存
// }
