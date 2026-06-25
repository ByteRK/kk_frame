/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/uart/uart_client.h
 * @Description: Raw UART client
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __UART_CLIENT_H__
#define __UART_CLIENT_H__

#include "transport.h"

#include <string>

class UartClient : public Transport {
public:
    struct Config {
        std::string device;
        int baudRate{ 9600 };
        int flowControl{ 0 };
        int dataBits{ 8 };
        int stopBits{ 1 };
        char parity{ 'N' };
        int pollIntervalMs{ 10 };
        int writeTimeoutMs{ 1000 };
        size_t readBufferSize{ 4096 };
    };

public:
    explicit UartClient(const Config& config);
    ~UartClient();

    void setHandler(TransportHandler* handler) override;
    int init() override;
    bool start() override;
    void stop() override;
    bool isConnected() const override;
    void onTick() override;
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

protected:
    void dispatchEvent(const Event& ev) override;

private:
    int openDevice();
    int configureDevice(int fd);
    ssize_t writeAll(const uint8_t* data, size_t len);

private:
    Config mConfig;
    int mFd;
    bool mRunning;
    int64_t mLastPollMs;
    TransportHandler* mHandler;
};

#endif // !__UART_CLIENT_H__
