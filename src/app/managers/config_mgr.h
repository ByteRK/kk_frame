/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-02-03 20:47:08
 * @FilePath: /kk_frame/src/app/managers/config_mgr.h
 * @Description: 配置管理
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef __CONFIG_MGR_H__
#define __CONFIG_MGR_H__

#include "template/singleton.h"
#include "class/auto_save.h"
#include <core/preferences.h>

#define g_config ConfigMgr::instance()

class ConfigMgr : public Singleton<ConfigMgr>,
    public AutoSaveItem {
    friend class Singleton<ConfigMgr>;
private:
    cdroid::Preferences mConfig;    // 配置文件[rw]
    cdroid::Preferences mDevConf;   // 设备配置，出厂烧写[ro]

private:
    ConfigMgr();

public:
    ~ConfigMgr();

    void init();
    void reset();

private:
    bool load();
    bool save(bool isBackup = false) override;
    bool haveChange() override;

public:
    std::string getProductId();  // 产品ID
    std::string getDeviceId();   // 设备ID
    std::string getLicense();    // 授权码
    void setDeviceConf(const std::string &_pid, const std::string &_did, const std::string &_lic);

public:
    // 亮度
    int  getBrightness();
    void setBrightness(int value);

    // 音量
    int  getVolume();
    void setVolume(int value);

    // WIFI
    bool getWifi();
    void setWifi(bool value);

    // WIFI SSID
    std::string getWifiSSID();
    void setWifiSSID(const std::string& value);

    // WIFI 密码
    std::string getWifiPassword();
    void setWifiPassword(const std::string& value);

    // 自动锁屏
    bool getAutoLock();
    void setAutoLock(bool value);
};


#endif // __CONFIG_MGR_H__