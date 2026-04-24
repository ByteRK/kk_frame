/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-04-24 15:38:29
 * @FilePath: /kk_frame/src/app/managers/config_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "config_mgr.h"
#include "config_info.h"
#include "file_utils.h"

#include <cdlog.h>
#include <unistd.h>

#define CONFIG_SECTION "app_config" // 配置文件节点

ConfigMgr::ConfigMgr() :
    AutoSaveItem(2000, 10000) {
}

ConfigMgr::~ConfigMgr() {
}

/// @brief 初始化
void ConfigMgr::init() {
    load();

    if (access(DEVCONF_FILE_PATH, F_OK) == 0) {
        LOGI("load %s", DEVCONF_FILE_PATH);
        mDevConf.load(DEVCONF_FILE_PATH);
    }

    AutoSaveItem::init();
}

/// @brief 重置
void ConfigMgr::reset() {
    std::string command = std::string("rm")\
        + " " + CONFIG_FILE_PATH + " " + CONFIG_FILE_BAK_PATH;
    std::system(command.c_str());
    FileUtils::sync();
    init();
    LOGE("config_mgr factory reset.");
}

/// @brief 从文件中加载配置
/// @return 
bool ConfigMgr::load() {
    mConfig = cdroid::Preferences(); // 清空原配置

    std::string loadingPath = "";
    size_t fileLen = 0;
    if (FileUtils::check(CONFIG_FILE_PATH, &fileLen) && fileLen > 0) {
        loadingPath = CONFIG_FILE_PATH;
    } else if (FileUtils::check(CONFIG_FILE_BAK_PATH, &fileLen) && fileLen > 0) {
        loadingPath = CONFIG_FILE_BAK_PATH;
    } else {
        LOG(ERROR) << "[config] no config file found. use default data.";
        mConfig.setValue(CONFIG_SECTION, "HELLO", std::string("WORLD!!!"));
        return false;
    }
    LOG(INFO) << "[config] load config. file=" << loadingPath;

    /**** 开始读取数据 ****/
    mConfig.load(loadingPath);
    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存配置到文件
/// @param isBackup 是否为备份
/// @return 
bool ConfigMgr::save(bool isBackup) {
    mConfig.save(
        isBackup ? CONFIG_FILE_BAK_PATH : CONFIG_FILE_PATH
    );
    return true;
}

/// @brief 检查是否存在变动，触发保存
/// @return 
bool ConfigMgr::haveChange() {
    return mConfig.getUpdates();
}

/**************************************************************************************/

std::string ConfigMgr::getProductId() {
    return mDevConf.getString(CONFIG_SECTION, "ProductId", "");
}

std::string ConfigMgr::getDeviceId() {
    return mDevConf.getString(CONFIG_SECTION, "DeviceId", "");
}

std::string ConfigMgr::getLicense() {
    return mDevConf.getString(CONFIG_SECTION, "License", "");
}

void ConfigMgr::setDeviceConf(const std::string& _pid, const std::string& _did, const std::string& _lic) {
    mDevConf.setValue(CONFIG_SECTION, "ProductId", _pid);
    mDevConf.setValue(CONFIG_SECTION, "DeviceId", _did);
    mDevConf.setValue(CONFIG_SECTION, "License", _lic);

    mDevConf.save(DEVCONF_FILE_PATH);
    sync(); // 立即同步
    LOGE("save read-only file. file=%s", DEVCONF_FILE_PATH);
}

/**************************************************************************************/

/// @brief 获取屏幕亮度
/// @return 
int ConfigMgr::brightness() {
    return mConfig.getInt(CONFIG_SECTION, "brightness", CONFIG_BRIGHTNESS);
}

/// @brief 设置屏幕亮度
/// @param value 
void ConfigMgr::brightness(int value) {
    if (value == brightness()) return;
    mConfig.setValue(CONFIG_SECTION, "brightness", value);
}

/// @brief 获取音量
/// @return 
int ConfigMgr::volume() {
    return mConfig.getInt(CONFIG_SECTION, "volume", CONFIG_VOLUME);
}

/// @brief 设置音量
/// @param value 
void ConfigMgr::volume(int value) {
    if (value == volume()) return;
    mConfig.setValue(CONFIG_SECTION, "volume", value);
}

/// @brief 获取自动锁屏
/// @return 
bool ConfigMgr::autoLock() {
    return mConfig.getBool(CONFIG_SECTION, "autoLock", CONFIG_AUTOLOCK);
}

/// @brief 设置自动锁屏
/// @param value 
void ConfigMgr::autoLock(bool value) {
    if (value == autoLock()) return;
    mConfig.setValue(CONFIG_SECTION, "autoLock", value);
}
