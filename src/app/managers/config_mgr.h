/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-02-02 16:42:14
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
    cdroid::Preferences mConfig;               // 配置文件

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