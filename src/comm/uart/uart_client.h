/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:47:22
 * @LastEditTime: 2026-07-05 20:57:08
 * @FilePath: /kk_frame/src/comm/uart/uart_client.h
 * @Description: 串口通讯客户端
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __UART_CLIENT_H__
#define __UART_CLIENT_H__

#include "tick_mgr.h"
#include "transport.h"

#include <mutex>
#include <set>
#include <string>

/// @brief 串口通讯客户端 [基于 POSIX 串口设备]
/// @note 短轮询间隔使用 FD 触发，较长轮询间隔使用 TickMgr 驱动
class UartClient : public Transport, private TickMgr::ITickClass {
public:
    /// @brief 串口参数配置
    struct Config {
        std::string device;                      // 串口设备路径
        int         baudRate{ 9600 };            // 波特率
        int         flowControl{ 0 };            // 流控模式: 0 无流控，1 硬件流控，2 软件流控
        int         dataBits{ 8 };               // 数据位
        int         stopBits{ 1 };               // 停止位
        char        parity{ 'N' };               // 校验位: N/n 无校验，O/o 奇校验，E/e 偶校验
        int         pollIntervalMs{ 200 };       // 读取调度间隔，单位毫秒（决定最终处理机制）
        int         writeTimeoutMs{ 1000 };      // 写入超时，单位毫秒，负数表示不设超时
        size_t      readBufferSize{ 4096 };      // 读取缓存大小，为 0 时使用 4096 字节
    };

private:
    static std::mutex            sDeviceLock;  // 设备占用记录锁
    static std::set<std::string> sUsedDevices; // 已占用的设备路径（防止重复打开同一设备）

private:
    Config          mConfig;                   // 通讯配置
    int             mFd{ -1 };                 // 串口文件描述符
    bool            mRunning{ false };         // 通道是否启动
    bool            mDeviceClaimed{ false };   // 当前实例是否已登记设备占用
    bool            mUseFdMode{ false };       // 是否使用 FD 触发模式
    bool            mFdRegistered{ false };    // 是否已注册 FD 事件
    cdroid::Looper* mLooper{ nullptr };        // FD 事件监听 Looper

public:
    explicit UartClient(const Config& config);
    ~UartClient();

    int     init()  override;
    bool    start() override;
    void    stop()  override;
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;
    bool    isConnected() const override;

private:
    void       onTick(int64_t nowMs) override;
    static int onFdEvent(int fd, int events, void* context);

private:
    int     handleFdEvent(int fd, int events);
    bool    startReadDriver();
    void    stopReadDriver();
    void    readAvailable();
    int     openDevice();
    int     configureDevice(int fd);
    void    releaseDeviceClaim();
    void    disconnectOnDeviceError(int error, int events);
    ssize_t writeAll(const uint8_t* data, size_t len);

};

#endif // !__UART_CLIENT_H__
