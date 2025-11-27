/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-11-27 11:44:12
 * @FilePath: /cy_frame/src/project/global_data.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#ifndef _GLOBAL_DATA_H_
#define _GLOBAL_DATA_H_

#include "struct.h"

#include <core/uieventsource.h>
#include <core/preferences.h>

enum {
    DEVICE_MODE_SAMPLE = 0,    // 常规模式
    DEVICE_MODE_TEST,          // 测试模式
    DEVICE_MODE_DEV,           // 开发模式
    DEVICE_MODE_DISPLAY,       // 演示模式

    DEVICE_MODE_MAX,
};

#define g_data globalData::ins()

class globalData :public MessageHandler {
public: // 特殊信息
    uint8_t     mDeviceMode = 0;                  // 设备模式

public: // 网络状态
    uint8_t     mNetWork = 0;                     // 网络状态
    uint8_t     mNetWorkDetail = 0;               // 网络详细状态

public: // 涂鸦部分
    bool        mTUYAPower = true;                // 电源状态
    int8_t      mTUYATem = 0;                     // 涂鸦温度
    int8_t      mTUYATemMin = 0;                  // 涂鸦温度最小值
    int8_t      mTUYATemMax = 0;                  // 涂鸦温度最大值
    std::string mTUYAWeather = "146";             // 涂鸦天气代码
    uint16_t    mTUYAWifiTestRes = 0xFFFF;        // wifi测试结果

public: // 设备信息
    bool        mPower = false;                   // 开关机
    bool        mLock = false;                    // 童锁

private:
    enum {
        MSG_SAVE,  // 备份检查消息
    };

    bool             mCoffee = false;             // 咖啡机[🎐演示保存逻辑的数据]
    Looper*          mLooper;                     // 消息循环
    bool             mHaveChange;                 // 是否需要保存
    uint64_t         mNextBakTime;                // 下次备份时间
    Message          mCheckSaveMsg;               // 备份检查消息
    uint64_t         mPowerOnTime;                // 启动时间[用于粗略计算运行时间]

private:
    globalData() = default;
public:
    ~globalData();
    static globalData* ins() {
        static globalData instance;
        return &instance;
    }
    globalData(const globalData&) = delete;       // 禁止拷贝构造
    globalData& operator=(globalData&) = delete;  // 禁止赋值构造
    globalData(globalData&&) = delete;            // 禁止移动构造
    globalData& operator=(globalData&&) = delete; // 禁止移动赋值构造

    void init();
    void reset();
    void handleMessage(Message& message)override;
private:
    void checkenv();
    bool loadFromFile();
    bool saveToFile(bool isBak = false);
    void checkToSave();
    
public:
    uint64_t getPowerOnTime();
    
};

#endif // _GLOBAL_DATA_H_