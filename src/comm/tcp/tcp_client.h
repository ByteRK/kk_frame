/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:24:24
 * @LastEditTime: 2026-02-07 10:36:19
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <cstddef>
#include <cstdint>

/// @brief TCP客户端事件处理基类
class ITcpClientHandler {
public:
    virtual ~ITcpClientHandler() { }

    virtual void onConnected() { }
    virtual void onDisconnected() { }
    virtual void onRecv(const uint8_t* data, size_t len) = 0;
    virtual void onError(int err) { }
};

#include "transport.h"
#include <thread>
#include <atomic>
#include <string>

/// @brief TCP客户端实现
class TcpClientTransport : public Transport {
public:
    TcpClientTransport(const std::string& ip, uint16_t port);
    ~TcpClientTransport();

    void setHandler(ITcpClientHandler* handler);

    void init();
    bool start() override;
    void stop() override;

    ssize_t send(const uint8_t* data, size_t len, int clientId = -1) override;

protected:
    void dispatchEvent(const TransportEvent& ev) override;

private:
    void threadLoop();
    bool connectServer();

private:
    std::string mIp;
    uint16_t    mPort;
    int         mSock{ -1 };

    std::thread mThread;
    std::atomic<bool> mRunning{ false };

    ITcpClientHandler* mHandler{ nullptr };
};

#endif // !__TCP_CLIENT_H__