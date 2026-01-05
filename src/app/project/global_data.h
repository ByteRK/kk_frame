/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-12-31 15:08:35
 * @FilePath: /kk_frame/src/app/project/global_data.h
 * @Description: 全局应用数据
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef __GLOBAL_DATA_H__
#define __GLOBAL_DATA_H__

#include "struct.h"
#include "template/singleton.h"

#include <core/uieventsource.h>
#include <core/preferences.h>

enum {
    DEVICE_MODE_DEMO = 0,      // 测试模式
    DEVICE_MODE_SAMPLE,        // 常规模式
    DEVICE_MODE_TEST,          // 测试模式
    DEVICE_MODE_DEV,           // 开发模式
    DEVICE_MODE_DISPLAY,       // 演示模式

    DEVICE_MODE_MAX,
};

#define g_data GlobalData::instance()

class GlobalData : public Singleton<GlobalData>,
    public MessageHandler {
    friend class Singleton<GlobalData>;
public: // 特殊信息
    bool           mIsFirstInit = true;               // 是否是首次初始化
    uint8_t        mDeviceMode = DEVICE_MODE_SAMPLE;  // 设备模式
    int            mTestPage = 0;                     // 测试页面
    const uint64_t mAppStart;                         // 应用启动时间

public: // 网络状态
    uint8_t        mNetWork = 0;                      // 网络状态
    uint8_t        mNetWorkDetail = 0;                // 网络详细状态

public: // 涂鸦部分
    bool        mTUYAPower = true;                    // 电源状态
    int8_t      mTUYATem = 0;                         // 涂鸦温度
    int8_t      mTUYATemMin = 0;                      // 涂鸦温度最小值
    int8_t      mTUYATemMax = 0;                      // 涂鸦温度最大值
    std::string mTUYAWeather = "146";                 // 涂鸦天气代码
    uint16_t    mTUYAWifiTestRes = 0xFFFF;            // wifi测试结果

public: // 设备信息
    bool        mPower = false;                       // 开关机
    bool        mLock = false;                        // 童锁

private:
    enum {
        MSG_SAVE,  // 备份检查消息
    };

    bool             mCoffee = false;                 // 咖啡机[🎐演示保存逻辑的数据]
    int              mArgc = 0;                       // 启动参数个数
    const char**     mArgv = nullptr;                 // 启动参数
    Looper*          mLooper;                         // 消息循环
    bool             mHaveChange;                     // 是否需要保存
    uint64_t         mNextBakTime;                    // 下次备份时间
    Message          mCheckSaveMsg;                   // 备份检查消息
    uint64_t         mPowerOnTime;                    // 启动时间[用于粗略计算运行时间]

private:
    GlobalData();

public:
    ~GlobalData();
    void init(int argc, const char* argv[]);
    void reset();
    void setFirstInit(bool first = true);
    void handleMessage(Message& message)override;

private:
    void checkenv();
    void checkArgv();
    bool loadFromFile();
    bool saveToFile(bool isBak = false);
    void checkToSave();

public:
    uint64_t getPowerOnTime();

};

#endif // !__GLOBAL_DATA_H__