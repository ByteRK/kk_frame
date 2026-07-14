/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-07-14 17:45:54
 * @FilePath: /kk_frame/src/app/data/global_data.h
 * @Description: 全局应用数据
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __GLOBAL_DATA_H__
#define __GLOBAL_DATA_H__

#include "base_data.h"
#include "template/singleton.h"
#include "class/auto_save.h"

typedef enum {
    DEVICE_MODE_DEMO = 0,      // 演示模式[模板使用,项目中正常不进入]
    
    DEVICE_MODE_SAMPLE,        // 常规模式[正常生产运行环境]
    DEVICE_MODE_DEV,           // 开发模式[一般为相关请求接口调用测试环境，开发使用]
    DEVICE_MODE_DISPLAY,       // 展示模式[一般为屏蔽故障，放开所有功能的模式，且相关工作跳转均正常，展会使用]
    DEVICE_MODE_TEST,          // 测试模式[定向进入某个页面，测试使用]

    DEVICE_MODE_MAX,
} DeviceMode;

#define g_data GlobalData::instance()

class GlobalData : public Singleton<GlobalData>,
    public BaseData, public AutoSaveItem {
    friend class Singleton<GlobalData>;
public:  // 特殊信息
    const uint64_t   mAppStart;                         // 应用启动时间
    DeviceMode       mDeviceMode = DEVICE_MODE_SAMPLE;  // 设备模式
    int              mTestPage = 0;                     // 测试页面
    bool             mHaveChange = false;               // 是否需要保存
    bool             mIsFirstInit = true;               // 是否是首次初始化

public:  // 设备信息
    bool             mPower = false;                    // 开关机
    bool             mLock = false;                     // 童锁

private: // 状态数据
    bool             mCoffee = false;                   // 咖啡机[🎐演示保存逻辑的数据]

private:
    GlobalData();

public:
    ~GlobalData();
    void init();
    void reset();
    void setFirstInit(bool first = true);

private:
    bool load();
    bool save(bool isBackup = false) override;
    bool haveChange() override;

public: // 项目数据交互
    // void updateCoffee(bool coffee);

};

#endif // !__GLOBAL_DATA_H__