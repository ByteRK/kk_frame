/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:27:49
 * @LastEditTime: 2026-02-07 14:08:15
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.cc
 * @Description: TCP客户端实现
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "tcp_client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <core/app.h>
#include <cdlog.h>

TcpClient::TcpClient(const std::string& ip, uint16_t port)
    : mIp(ip), mPort(port) {
    LOGI("TcpClient::TcpClient(%s, %d)", ip.c_str(), port);
}

TcpClient::~TcpClient() {
    stop();
}

void TcpClient::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

void TcpClient::init() {
    cdroid::App::getInstance().addEventHandler(this);
}

bool TcpClient::start() {
    if (mRunning)
        return false;

    mRunning = true;
    mThread = std::thread(&TcpClient::threadLoop, this);
    return true;
}

void TcpClient::stop() {
    if (!mRunning)
        return;

    mRunning = false;

    if (mSock >= 0) {
        shutdown(mSock, SHUT_RDWR);
        close(mSock);
        mSock = -1;
    }

    if (mThread.joinable())
        mThread.join();
}

bool TcpClient::connectServer() {
    mSock = socket(AF_INET, SOCK_STREAM, 0);
    if (mSock < 0)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mPort);
    inet_pton(AF_INET, mIp.c_str(), &addr.sin_addr);

    if (connect(mSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(mSock);
        mSock = -1;
        return false;
    }

    Transport::Event ev;
    ev.type = Transport::Event::CONNECTED;
    postEvent(ev);
    return true;
}

void TcpClient::threadLoop() {
    while (mRunning) {
        if (!connectServer()) {
            sleep(2);           // 防止空转
            continue;
        }

        uint8_t buf[1024];
        while (mRunning) {
            ssize_t n = recv(mSock, buf, sizeof(buf), 0);
            if (n <= 0)
                break;

            Transport::Event ev;
            ev.type = Transport::Event::DATA;
            ev.data.assign(buf, buf + n);
            postEvent(ev);
        }

        if (mSock >= 0) {
            close(mSock);
            mSock = -1;
        }

        Transport::Event ev;
        ev.type = Transport::Event::DISCONNECTED;
        postEvent(ev);

        sleep(2);   // 重连节流
    }
}

ssize_t TcpClient::send(const uint8_t* data, size_t len, int) {
    if (mSock < 0)
        return -1;
    return ::send(mSock, data, len, 0);
}

void TcpClient::dispatchEvent(const Transport::Event& ev) {
    if (!mHandler)
        return;

    switch (ev.type) {
    case Transport::Event::CONNECTED:
        mHandler->onConnected();
        break;
    case Transport::Event::DISCONNECTED:
        mHandler->onDisconnected();
        break;
    case Transport::Event::DATA:
        mHandler->onRecv(ev.data.data(), ev.data.size());
        break;
    case Transport::Event::ERROR:
        mHandler->onError(ev.error);
        break;
    default:
        break;
    }
}
