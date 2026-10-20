/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-08-14 15:59:12
 * @FilePath: /kk_frame/src/app/data/global_data.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "global_data.h"
#include "arg_utils.h"
#include "env_utils.h"
#include "json_utils.h"
#include "file_utils.h"
#include "base_data.h"
#include "config_info.h"
#include "system_utils.h"
#include "base.h"

#include <unistd.h>
#include <core/app.h>
#include <core/systemclock.h>
#include <cdlog.h>

/**
 * 为了触发DEMO模式使用
**/
#define FRAME_NAME "kk" "_frame"

GlobalData::GlobalData() :
    mAppStart(cdroid::SystemClock::uptimeMillis()),
    AutoSaveItem(2000, 10000) { }

/// @brief 析构
GlobalData::~GlobalData() { }

/// @brief 初始化
void GlobalData::init() {
    mIsFirstInit = !FileUtils::check(APP_FIRST_TAG);

    // 恢复系统时间
    if (mIsFirstInit) SystemUtils::clearTimeCache();
    else SystemUtils::recoverTime();

    // 检测进入的模式
    if (cdroid::App::getInstance().getName() == FRAME_NAME) mDeviceMode = DEVICE_MODE_DEMO;
    mDeviceMode = EnvUtils::getEnumOr<DeviceMode>("DEVICE_MODE", mDeviceMode);
    if (ArgUtils::get().isDemo) mDeviceMode = DEVICE_MODE_DEMO;
    if ((mTestPage = ArgUtils::get().selectPage) != PAGE_NULL) mDeviceMode = DEVICE_MODE_TEST;
    LOGW_IF(mDeviceMode, "DEVICE_MODE: %d, TEST_PAGE: %d", mDeviceMode, mTestPage);

    load();

    AutoSaveItem::init();
}

/// @brief 重置
void GlobalData::reset() {
    FileUtils::remove(APP_FILE_PATH, APP_FILE_BAK_PATH);
    setFirstInit(true);
    // FileUtils::sync(); // 不需要Sync，上一步已Sync
    init();
    mHaveChange = true;
    LOGE("global_data factory reset.");
}

/// @brief 设置首次初始化状态
/// @param first true:删除已初始化标记 false:创建已初始化标记
void GlobalData::setFirstInit(bool first) {
    if (first) {
        FileUtils::remove(APP_FIRST_TAG);
    } else if (!FileUtils::write(APP_FIRST_TAG, std::string())) {
        LOGE("create first-init tag failed. file=%s", APP_FIRST_TAG);
    }

    mIsFirstInit = first;
    FileUtils::sync();
}

/// @brief 载入本地文件
/// @return 
bool GlobalData::load() {
    Json::Value appJson;
    std::string loadingPath;

    const bool res = JsonUtils::load(
        APP_FILE_PATH, APP_FILE_BAK_PATH, appJson, &loadingPath,
        [](const Json::Value& value) { return value.isObject(); });

    if (res) {
        LOG(INFO) << "[app] load local data. file=" << loadingPath;
    } else {
        LOG(ERROR) << "[app] no valid local data file found. use default data";
        mHaveChange = true;
        return false;
    }

    /**** 开始读取数据 ****/
    mCoffee = JsonUtils::get(appJson, "coffee", true);
    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存文件到本地
/// @param isBackup 是否为备份
/// @return 成功返回true
/// @note 一般只建议保存需要离电记忆的页面状态数据
bool GlobalData::save(bool isBackup) {
    Json::Value appJson;
    /**** 开始写入数据 ****/
    appJson["coffee"] = mCoffee;
    /**** 结束写入数据 ****/

    // 写盘成功后才复位标记，避免写失败后数据不再重试
    if (!JsonUtils::save(APP_FILE_PATH, APP_FILE_BAK_PATH, appJson, isBackup)) {
        LOGE("global_data save failed. file=%s",
            isBackup ? APP_FILE_BAK_PATH : APP_FILE_PATH);
        return false;
    }

    mHaveChange = false;
    return true;
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
