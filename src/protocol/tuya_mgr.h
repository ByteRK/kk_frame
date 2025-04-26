/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-06-20 15:14:05
 * @LastEditTime: 2025-01-19 16:43:53
 * @FilePath: /kaidu_tr_pro/src/protocol/tuya_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
 */

#ifndef __tuya_mgr_h__
#define __tuya_mgr_h__

#include "packet_buffer.h"
#include "uart_client.h"
#include "packet_handler.h"

#include "common.h"
#include "struct.h"

#define g_tuyaMgr TuyaMgr::ins()

class TuyaMgr : public EventHandler, public IHandler {
private: // 涂鸦数据点缓存
    bool             mPower = true;          // 开关

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
    
    /// @brief 处理下发的DP
    void acceptDP(uint8_t* data, uint16_t len);

    /// @brief 处理下发的时间
    /// @param status 
    /// @param data 
    void acceptTime(uint8_t* data);

    /// @brief 处理下发的天气
    /// @param data 
    /// @param len 
    void acceptWeather(uint8_t* data, uint16_t len);

    /// @brief 处理开启时间服务结果
    /// @param data 
    void acceptOpenTime(uint8_t* data);

    /// @brief 处理开启天气服务结果
    void acceptOpenWeather(uint8_t* data);
private:

private:
    /// @brief 处理涂鸦OTA开始
    /// @param data 
    void dealOTAComm(uint8_t* data, uint16_t len);

    /// @brief 处理涂鸦OTA数据
    /// @param data 
    void dealOTAData(uint8_t* data, uint16_t len);
};

#endif
