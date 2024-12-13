/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-20 15:14:05
 * @LastEditTime: 2024-11-13 16:20:40
 * @FilePath: /kaidu_t2e_pro/src/protocol/tuya_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef __tuya_mgr_h__
#define __tuya_mgr_h__

#include "packet_buffer.h"
#include "uart_client.h"
#include "cmd_handler.h"
#include "global_data.h"

#include "common.h"
#include "struct.h"

#define g_tuyaMgr TuyaMgr::ins()

class TuyaMgr : public EventHandler, public IHandler {
private: // 涂鸦数据点缓存
    bool             mPower = true;          // 开关
    uint8_t          mFunLevel = 0;          // 风机档位
    bool             mLock = false;          // 锁
    bool             mLamp = false;          // 烟机照明
    bool             mStart = false;         // 蒸烤启动
    uint32_t         mReserveTime = 0;       // 预约烹饪结束时间
    uint32_t         mAllTime = 0;           // 烹饪时间
    uint32_t         mOverTime = 0;          // 剩余时间
    uint32_t         mSetTem = 0;            // 烹饪温度
    uint32_t         mNowTem = 0;            // 当前温度
    uint32_t         mFault = 0;             // 故障警告
    uint32_t         mMode = 0;              // 蒸烤箱模式
    bool             mLamp2 = false;         // 蒸烤炉灯
    uint8_t          mStatus = 0;            // 蒸烤炉状态
    uint8_t          mLHobStatus = 0;        // 左灶具状态
    uint8_t          mRHobStatus = 0;        // 右灶具状态
    uint32_t         mLeftTimer = 0;         // 左灶具定时
    uint32_t         mRightTimer = 0;        // 右灶具定时
    bool             mClean = false;         // 烟机清洗
    bool             mInsulation = false;    // 烟机保温
    uint32_t         mRecipe = 0;            // 菜谱模式
    uint32_t         mDownTem = 0;           // 下温度
    uint32_t         mUpTem = 0;             // 上温度
    uint8_t          mLTimerStatus = 2;      // 左灶具定时状态 | 2 无定时 1 定时结束 0 定时中
    uint8_t          mRTimerStatus = 2;      // 左灶具定时状态 | 2 无定时 1 定时结束 0 定时中
    uint32_t         mSmokerDisplay = 0;     // 烟机延时显示
    uint8_t          mDiyMode = 0;           // DIY菜谱
    uint8_t          mInsulationTime = 0;    // 顶部保温时间
    uint8_t          mAirTime[5] = {0,0,0,5,0};      // 定时新风时间
    uint8_t          mCleanStatus = 0;       // 烟机洁净度提醒
    uint8_t          mBootMode = 2;          // 烟机联动设置
    uint8_t          mSmokerDelay = 2;       // 烟机延时设置
    bool             mSmokerRun = true;      // 烟机延时运行
    bool             mAuto = false;          // 自动巡航
    uint8_t          mAlart = 0;             // 特殊提醒

    int8_t           mDiyStep = -1;           // DIY菜谱步骤
    bool             mDiyChange = false;      // DIY菜谱变化

private:
    bool             mClearWifi = false;    // 复位WIFI标志位

    IPacketBuffer*   mPacket;               // 数据包
    int64_t          mNextEventTime;        // 下一次发包时间
    int64_t          mLastSendTime;         // 最后一次发送数据时间
    int64_t          mLastAcceptTime;       // 最后一次接受数据时间
    int64_t          mLastSendDiffDPTime;   // 最后一次差异上报时间
    int64_t          mLastSyncDateTime;     // 最后一次时间同步时间
    UartClient*      mUartTUYA;             // 涂鸦服务

    bool             mIsRunConnectWork;     // 是否已完成联网
    int64_t          mNetWorkConnectTime;   // 联网成功时间

    bool             mStartCache = false;
    uint32_t         mSetTemCache = 0;
    int32_t          mReserveTimeCache = -1;
    uint32_t         mAllTimeCache = 0;
    uint32_t         mStatusCache = 0;
    uint32_t         mModeCache = 0;
    uint32_t         mRecipeCache = 0;
    SmartStruct      mDiyCache;
    uint32_t         mUpTemCache = 0;
    uint32_t         mDownTemCache = 0;
    int32_t          mReserveTimeReset = -1;
    std::string      mReserveDateReset = "";

    uint32_t         mOTALen = 0;           // OTA数据长度
    uint32_t         mOTACurLen = 0;        // OTA当前接收长度
    uint64_t         mOTAAcceptTime = 0;    // OTA接收数据时间
protected:
    TuyaMgr();
    ~TuyaMgr();

public:
    static TuyaMgr* ins() {
        static TuyaMgr stIns;
        return &stIns;
    }

    int init();

protected:
    virtual int checkEvents();
    virtual int handleEvents();

    // 发送给MCU(不带消息)
    void send2MCU(uint8_t cmd);

    // 发送给MCU
    void send2MCU(uint8_t* buf, uint16_t len, uint8_t cmd);

    // 处理数据
    virtual void onCommDeal(IAck* ack);

public:
    /// @brief 心跳
    void sendHeartBeat();

    /// @brief 开始WiFi测试
    void sendWifiTest();

    /// @brief 重置wifi
    void resetWifi(bool clear = false);

    /// @brief 设置配网模式
    /// @param mode  0：默认配网 1：低功耗 2：特殊配网
    void sendSetConnectMode(uint8_t mode = 0);

    /// @brief 设置模块的工作模式
    void sendSetWorkMode();

    /// @brief 回复wifi状态
    void sendWIFIStatus(uint8_t status);

    /// @brief 获取涂鸦时间
    void getTuyaTime();

    /// @brief 打开时间服务
    void openTimeServe();

    /// @brief 打开天气服务
    void openWeatherServe();

    /// @brief 发送全部DP
    void sendDp();

    /// @brief 发送差分DP
    void sendDiffDp();

    /// @brief 获取OTA进度
    /// @return 
    uint8_t getOTAProgress();

    /// @brief 获取OTA数据接收时间
    /// @return 
    uint64_t getOTAAcceptTime();

private:
    /// @brief 设置dp数据
    /// @param buf 
    /// @param count 
    /// @param dp 
    /// @param type 
    /// @param data 
    /// @param dlen 
    /// @param reverse 
    void createDp(uint8_t* buf, uint16_t& count, uint8_t dp, uint8_t type, void* data, uint16_t dlen, bool reverse = true);

    /// @brief 设置DIY dp数据
    /// @param buf 
    /// @param count 
    void createDIYDp(uint8_t* buf, uint16_t& count);

    /// @brief 设置定时新风 dp数据
    /// @param buf 
    /// @param count 
    void createAirTimerDp(uint8_t* buf, uint16_t& count);

    /// @brief 设置灶具状态 dp数据
    /// @param buf 
    /// @param count 
    void createHobDp(uint8_t* buf, uint16_t& count);
    
    /// @brief 处理下发的DP
    void acceptDP(uint8_t* data, uint16_t len);

    /// @brief 处理下发的时间
    /// @param status 
    /// @param data 
    void acceptTime(uint8_t* data);

    /// @brief 处理开启天气服务结果
    void acceptOpenWeather(uint8_t* data);

    /// @brief 处理下发的天气
    /// @param data 
    /// @param len 
    void acceptWeather(uint8_t* data, uint16_t len);

    /// @brief 处理开启时间服务结果
    /// @param data 
    void acceptOpenTime(uint8_t* data);

private:
    /// @brief 处理diy数据
    /// @param data 
    /// @param len 
    void dealDiy(uint8_t* data, uint16_t len, bool isRunning = false);

public:
    /// @brief 重置涂鸦状态计数
    void resetTuyaRun();

private:
    /// @brief 检查涂鸦是否发送开始工作命令
    void checkTuyaRun();

    /// @brief 检查涂鸦发送的工作命令
    /// @return 
    bool checkTuyaRunData(bool setStatus = true, bool isReserve = false);

    /// @brief 检测预约时间重置
    void checkTuyaReserveReset();

private:
    /// @brief 处理涂鸦OTA开始
    /// @param data 
    void dealOTAComm(uint8_t* data, uint16_t len);

    /// @brief 处理涂鸦OTA数据
    /// @param data 
    void dealOTAData(uint8_t* data, uint16_t len);
};

#endif
