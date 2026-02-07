/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:24:24
 * @LastEditTime: 2026-02-07 14:07:42
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.h
 * @Description: TCP客户端实现
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "transport.h"
#include "tcp_handler.h"
#include <thread>
#include <atomic>
#include <string>

/// @brief TCP客户端实现
class TcpClientTransport : public Transport {
public:
    TcpClientTransport(const std::string& ip, uint16_t port);
    ~TcpClientTransport();

    void setHandler(TcpHandler* handler);

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

    TcpHandler* mHandler{ nullptr };
};

#endif // !__TCP_CLIENT_H__