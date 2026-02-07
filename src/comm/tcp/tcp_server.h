/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:25:46
 * @LastEditTime: 2026-02-07 16:57:05
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.h
 * @Description: TCP服务端实现
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "transport.h"
#include "transport_handler.h"
#include <thread>
#include <atomic>
#include <map>

/// @brief TCP服务端实现
class TcpServer : public Transport {
public:
    explicit TcpServer(uint16_t port);
    ~TcpServer();

    void setHandler(TransportHandler* handler);

    void init();
    bool start() override;
    void stop() override;

    ssize_t send(const uint8_t* data, size_t len, int id) override;

protected:
    void dispatchEvent(const Transport::Event& ev) override;

private:
    void threadLoop();
    int  createListenSocket();

private:
    uint16_t mPort;
    int mListenSock{-1};

    std::map<int, int> mClients; // id -> fd
    int mNextClientId{1};

    std::thread mThread;
    std::atomic<bool> mRunning{false};

    TransportHandler* mHandler{nullptr};
};

#endif // !__TCP_SERVER_H__
