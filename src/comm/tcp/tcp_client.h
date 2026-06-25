/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.h
 * @Description: Raw TCP client
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "transport.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class TcpClient : public Transport {
public:
    struct Config {
        std::string host;
        uint16_t port{ 0 };
        int reconnectDelayMs{ 2000 };
        size_t readBufferSize{ 4096 };
    };

public:
    TcpClient(const std::string& host, uint16_t port);
    explicit TcpClient(const Config& config);
    ~TcpClient();

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
    int connectServer();
    void closeSocket();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);

private:
    Config mConfig;
    mutable std::mutex mSocketLock;
    int mSock;
    std::thread mThread;
    std::atomic<bool> mRunning;
    std::atomic<bool> mConnected;
    TransportHandler* mHandler;
};

#endif // !__TCP_CLIENT_H__
