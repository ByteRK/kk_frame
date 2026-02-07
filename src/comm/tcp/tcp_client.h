/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:24:24
 * @LastEditTime: 2026-02-07 16:56:42
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
#include "transport_handler.h"
#include <thread>
#include <atomic>
#include <string>

/// @brief TCP客户端实现
class TcpClient : public Transport {
public:
    TcpClient(const std::string& ip, uint16_t port);
    ~TcpClient();

    void setHandler(TransportHandler* handler);

    void init();
    bool start() override;
    void stop() override;

    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

protected:
    void dispatchEvent(const Transport::Event& ev) override;

private:
    void threadLoop();
    bool connectServer();

private:
    std::string mIp;
    uint16_t    mPort;
    int         mSock{ -1 };

    std::thread mThread;
    std::atomic<bool> mRunning{ false };

    TransportHandler* mHandler{ nullptr };
};

#endif // !__TCP_CLIENT_H__