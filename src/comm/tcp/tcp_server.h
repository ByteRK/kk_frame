/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:25:46
 * @LastEditTime: 2026-02-07 10:34:41
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <cstddef>
#include <cstdint>

/// @brief TCP客户端事件处理基类
class ITcpServerHandler {
public:
    virtual ~ITcpServerHandler() { }

    virtual void onConnected(int clientId) { }
    virtual void onDisconnected(int clientId) { }

    virtual void onRecv(int clientId,
        const uint8_t* data, size_t len) = 0;
};

#include "transport.h"
#include <thread>
#include <atomic>
#include <map>

/// @brief TCP服务端实现
class TcpServerTransport : public Transport {
public:
    explicit TcpServerTransport(uint16_t port);
    ~TcpServerTransport();

    void setHandler(ITcpServerHandler* handler);

    void init();
    bool start() override;
    void stop() override;

    ssize_t send(const uint8_t* data, size_t len, int clientId) override;

protected:
    void dispatchEvent(const TransportEvent& ev) override;

private:
    void threadLoop();
    int  createListenSocket();

private:
    uint16_t mPort;
    int mListenSock{-1};

    std::map<int, int> mClients; // clientId -> fd
    int mNextClientId{1};

    std::thread mThread;
    std::atomic<bool> mRunning{false};

    ITcpServerHandler* mHandler{nullptr};
};

#endif // !__TCP_SERVER_H__
