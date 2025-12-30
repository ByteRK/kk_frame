/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-12-29 12:07:05
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
#include <core/handler.h>
#include <core/uieventsource.h>
#include <core/preferences.h>

#define g_config ConfigMgr::instance()

class ConfigMgr : public Singleton<ConfigMgr>,
    public MessageHandler {
    friend class Singleton<ConfigMgr>;
private:
    enum {
        MSG_SAVE,  // 备份检查消息
    };

    Looper*          mLooper;                  // 消息循环
    uint64_t         mNextBakTime;             // 下次备份时间
    Message          mCheckSaveMsg;            // 备份检查消息
    uint64_t         mPowerOnTime;             // 启动时间[用于粗略计算运行时间]
    cdroid::Preferences mConfig;               // 配置文件

private:
    ConfigMgr() = default;
public:
    ~ConfigMgr();

    void init();
    void reset();
    void handleMessage(Message& message)override;
private:
    bool loadFromFile();
    void checkToSave();

public:
    // 亮度
    int  getBrightness();
    void setBrightness(int value);

    // 音量
    int  getVolume();
    void setVolume(int value);

    // 自动锁屏
    bool getAutoLock();
    void setAutoLock(bool value);
};


#endif // __CONFIG_MGR_H__