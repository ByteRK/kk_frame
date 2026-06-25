/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.h
 * @Description: Raw TCP server
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "transport.h"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

class TcpServer : public Transport {
public:
    struct Config {
        uint16_t port{ 0 };
        int backlog{ 8 };
        size_t readBufferSize{ 4096 };
    };

public:
    explicit TcpServer(uint16_t port);
    explicit TcpServer(const Config& config);
    ~TcpServer();

    void setHandler(TransportHandler* handler) override;
    int init() override;
    bool start() override;
    void stop() override;
    bool isConnected() const override;
    void onTick() override { }
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

protected:
    void dispatchEvent(const Event& ev) override;

private:
    void threadLoop();
    int createListenSocket();
    void closeAllSockets();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);

private:
    Config mConfig;
    int mListenSock;
    std::map<int, int> mClients;
    int mNextClientId;
    mutable std::mutex mSocketLock;
    std::thread mThread;
    std::atomic<bool> mRunning;
    TransportHandler* mHandler;
};

#endif // !__TCP_SERVER_H__
