/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-11-27 11:44:00
 * @FilePath: /cy_frame/src/project/config_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#ifndef __CONFIG_MGR_H__
#define __CONFIG_MGR_H__

#include <core/handler.h>
#include <core/uieventsource.h>
#include <core/preferences.h>
#include "common.h"

#define g_config configMgr::ins()

class configMgr :public MessageHandler {
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
    configMgr() = default;
public:
    ~configMgr();
    static configMgr* ins() {
        static configMgr instance;
        return &instance;
    }
    configMgr(const configMgr&) = delete;       // 禁止拷贝构造
    configMgr& operator=(configMgr&) = delete;  // 禁止赋值构造
    configMgr(configMgr&&) = delete;            // 禁止移动构造
    configMgr& operator=(configMgr&&) = delete; // 禁止移动赋值构造

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