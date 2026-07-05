/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:44:35
 * @LastEditTime: 2026-07-05 21:01:20
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.h
 * @Description: TCP通讯客户端
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "transport.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

/// @brief Tcp通讯客户端 [带重连]
/// @note 基于 AsyncTransport 的 FD 模式实现
class TcpClient : public AsyncTransport {
public:
    /// @brief TCP 客户端配置
    struct Config {
        std::string host;                      // IP 地址或主机名
        uint16_t    port{ 0 };                 // 服务端端口
        int         sendTimeoutMs{ 1000 };     // 发送超时，单位毫秒
        int         reconnectDelayMs{ 2000 };  // 重连间隔，单位毫秒
        size_t      readBufferSize{ 4096 };    // 读取缓存大小，为 0 时使用 4096 字节
    };

private:
    Config                  mConfig;             // 通讯配置
    std::mutex              mSendLock;           // 发送锁
    int                     mSock{ -1 };         // 通讯套接字
    mutable std::mutex      mSocketLock;         // 通讯套接字锁
    std::thread             mThread;             // 通讯工作线程
    std::atomic<bool>       mRunning{ false };   // 是否正在运行
    std::atomic<bool>       mConnected{ false }; // 是否已连接
    std::mutex              mStopLock;           // 停止锁
    std::condition_variable mStopCondition;      // 停止条件变量

public:
    TcpClient(const std::string& host, uint16_t port);
    explicit TcpClient(const Config& config);
    ~TcpClient();

    int     init() override;
    bool    start() override;
    void    stop() override;
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;
    bool    isConnected() const override;

private:
    void    threadLoop();
    int     connectServer();
    void    waitForReconnectDelay();
    void    closeSocket();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);
};

#endif // !__TCP_CLIENT_H__
