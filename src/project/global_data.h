/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2025-02-20 22:24:01
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

#define g_data globalData::ins()

class globalData :public MessageHandler {
public:
    uint8_t     mNetWork = 0;             // 网络状态
    uint8_t     mNetWorkDetail = 0;       // 网络详细状态

public: // 涂鸦部分
    bool        mTUYAPower = true;        // 电源状态
    int8_t      mTUYATem = 0;             // 涂鸦温度
    int8_t      mTUYATemMin = 0;          // 涂鸦温度最小值
    int8_t      mTUYATemMax = 0;          // 涂鸦温度最大值
    std::string mTUYAWeather = "146";     // 涂鸦天气代码
    uint16_t    mWifiTestRes = 0xFFFF;    // wifi测试结果

public: // 设备信息
    bool        mPower = false;           // 开关机
    bool        mLock = false;            // 童锁
private:
    enum {
        MSG_SAVE,
    };

    Looper*          mLooper;
    bool             mHaveChange;
    uint64_t         mNextBakTime;

    Message          mSaveMsg;
    uint64_t         mPowerOnTime;  // 启动时间

private:
    globalData();
    void update();
public:
    ~globalData();
    static globalData* ins() {
        static globalData s_globalData;
        return &s_globalData;
    }
    void handleMessage(Message& message)override;

    void init();

    uint64_t getPowerOnTime();
private:
    /*本地信息*/
    bool loadFromFile();
    bool saveToFile(bool isBak = false);
public:
};

#endif // _GLOBAL_DATA_H_