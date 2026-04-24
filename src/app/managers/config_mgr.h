/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-04-24 15:37:19
 * @FilePath: /kk_frame/src/app/managers/config_mgr.h
 * @Description: 配置管理
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

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

public: // 设备配置
    std::string getProductId();  // 产品ID
    std::string getDeviceId();   // 设备ID
    std::string getLicense();    // 授权码
    void        setDeviceConf(const std::string &_pid, const std::string &_did, const std::string &_lic);

public: // 应用配置
    // 亮度
    int  brightness();
    void brightness(int value);

    // 音量
    int  volume();
    void volume(int value);

    // 自动锁屏
    bool autoLock();
    void autoLock(bool value);
};


#endif // __CONFIG_MGR_H__