/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-20 15:14:05
 * @LastEditTime: 2026-07-14 23:22:14
 * @FilePath: /kk_frame/src/app/protocol/tuya_mgr.h
 * @Description: 涂鸦模组通讯
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TUYA_MGR_H__
#define __TUYA_MGR_H__

#include "template/singleton.h"
#include "packet_channel.h"
#include "packet_mgr.h"
#include "tick_mgr.h"

#include "struct.h"

#include <atomic>
#include <stddef.h>
#include <string>
#include <vector>

#ifdef PRODUCT_X64
#include "tcp_client.h"
typedef PacketChannel<TcpClient>  TuyaCommChannel;
#else
#include "uart_client.h"
typedef PacketChannel<UartClient> TuyaCommChannel;
#endif

#define g_tuyaMgr TuyaMgr::instance()

class TuyaMgr : public TickMgr::ITickClass, public PacketHandler,
    public Singleton<TuyaMgr> {
    friend Singleton<TuyaMgr>;
public:
    enum {
        WIFI_NULL = 0,
        WIFI_1,
        WIFI_2,
        WIFI_3,
        WIFI_4,
        WIFI_ERROR,
    };

public:  // 涂鸦网络数据点
    bool              mNetOK = false;             // 网络是否正常(连接成功)
    uint8_t           mNetWork = 0;               // 网络状态(信号)
    uint8_t           mNetWorkDetail = 0;         // 网络详细状态

public:  // 涂鸦数据点
    bool              mPower = true;              // 电源状态
    int8_t            mTem = 0;                   // 涂鸦温度
    int8_t            mTemMin = 0;                // 涂鸦温度最小值
    int8_t            mTemMax = 0;                // 涂鸦温度最大值
    std::string       mWeather = "146";           // 涂鸦天气代码
    uint16_t          mWifiTestRes = 0xFFFF;      // wifi测试结果
    
private: // 涂鸦数据点缓存
    bool              mCachePower = true;         // 开关

private:
    TuyaCommChannel*  mChannel{ nullptr };        // 涂鸦通讯通道
    PacketBufferPool* mPacket{ nullptr };         // 数据包缓存池

private:
    std::atomic<bool> mInitialized{ false };      // 初始化完成标志
    int64_t           mLastSendTime{ 0 };         // 最后一次发送数据时间
    int64_t           mLastAcceptTime{ 0 };       // 最后一次接受数据时间
    int64_t           mLastSendDiffDPTime{ 0 };   // 最后一次差异上报时间
    int64_t           mLastSyncDateTime{ 0 };     // 最后一次时间同步时间

    bool              mClearWifi = false;         // 复位WIFI标志位
    bool              mIsRunConnectWork{ false }; // 是否已完成联网
    int64_t           mNetWorkConnectTime{ 0 };   // 联网成功时间

    uint64_t          mOTAAcceptTime = 0;         // 最近一次被拒绝的OTA请求时间

protected:
    TuyaMgr();
    ~TuyaMgr();

public:
    int init();

protected:
    void onTick(int64_t nowMs) override;

    // 发送给MCU(不带消息)
    bool send2MCU(uint8_t cmd);

    // 发送给MCU
    bool send2MCU(const uint8_t* buf, size_t len, uint8_t cmd);

    // 处理数据
    virtual void onCommDeal(const IAck* ack) override;

public:
    /// @brief 心跳
    bool sendHeartBeat();

    /// @brief 开始WiFi测试
    bool sendWifiTest();

    /// @brief 重置wifi
    bool resetWifi(bool clear = false);

    /// @brief 设置配网模式
    /// @param mode  0：默认配网 1：低功耗 2：特殊配网
    bool sendSetConnectMode(uint8_t mode = 0);

    /// @brief 设置模块的工作模式
    bool sendSetWorkMode();

    /// @brief 回复wifi状态
    bool sendWIFIStatus(uint8_t status);

    /// @brief 获取涂鸦时间
    bool getTuyaTime();

    /// @brief 打开时间服务
    bool openTimeServe();

    /// @brief 打开天气服务
    bool openWeatherServe();

    /// @brief 发送全部DP
    bool sendDp();

    /// @brief 发送差分DP
    bool sendDiffDp();

    /// @brief 获取OTA进度
    /// @return 
    uint8_t getOTAProgress();

    /// @brief 获取OTA数据接收时间
    /// @return 
    uint64_t getOTAAcceptTime();

private:
    /// @brief 向有界载荷追加一个DP
    bool appendDp(std::vector<uint8_t>& payload, uint8_t dp, uint8_t type,
        const void* data, size_t dlen, bool reverse = true);

    /// @brief 处理下发的DP
    bool acceptDP(const uint8_t* data, uint16_t len);

    /// @brief 处理下发的时间
    /// @param data 数据载荷
    /// @param len 数据载荷长度
    bool acceptTime(const uint8_t* data, uint16_t len);

    /// @brief 处理下发的天气
    /// @param data 
    /// @param len 
    bool acceptWeather(const uint8_t* data, uint16_t len);

    /// @brief 处理开启时间服务结果
    /// @param data 数据载荷
    /// @param len 数据载荷长度
    bool acceptOpenTime(const uint8_t* data, uint16_t len);

    /// @brief 处理开启天气服务结果
    bool acceptOpenWeather(const uint8_t* data, uint16_t len);

private:
    /// @brief 拒绝未具备可信校验链路的远程OTA命令
    void rejectOTA(uint8_t command, uint16_t len);
};

#endif // !__TUYA_MGR_H__
