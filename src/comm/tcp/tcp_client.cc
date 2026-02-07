/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:27:49
 * @LastEditTime: 2026-02-07 10:46:14
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.cc
 * @Description:
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

TcpClientTransport::TcpClientTransport(const std::string& ip, uint16_t port)
    : mIp(ip), mPort(port) {
}

TcpClientTransport::~TcpClientTransport() {
    stop();
}

void TcpClientTransport::setHandler(ITcpClientHandler* handler) {
    mHandler = handler;
}

void TcpClientTransport::init() {
    cdroid::App::getInstance().addEventHandler(this);
}

bool TcpClientTransport::start() {
    if (mRunning)
        return false;

    mRunning = true;
    mThread = std::thread(&TcpClientTransport::threadLoop, this);
    return true;
}

void TcpClientTransport::stop() {
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

bool TcpClientTransport::connectServer() {
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

    TransportEvent ev;
    ev.type = TransportEvent::CONNECTED;
    postEvent(ev);
    return true;
}

void TcpClientTransport::threadLoop() {
    while (mRunning) {
        if (!connectServer()) {
            sleep(3);           // 防止空转
            continue;
        }

        uint8_t buf[1024];
        while (mRunning) {
            ssize_t n = recv(mSock, buf, sizeof(buf), 0);
            if (n <= 0)
                break;

            TransportEvent ev;
            ev.type = TransportEvent::DATA;
            ev.data.assign(buf, buf + n);
            postEvent(ev);
        }

        if (mSock >= 0) {
            close(mSock);
            mSock = -1;
        }

        TransportEvent ev;
        ev.type = TransportEvent::DISCONNECTED;
        postEvent(ev);

        sleep(3);   // 重连节流
    }
}

ssize_t TcpClientTransport::send(const uint8_t* data, size_t len, int) {
    if (mSock < 0)
        return -1;
    return ::send(mSock, data, len, 0);
}

void TcpClientTransport::dispatchEvent(const TransportEvent& ev) {
    if (!mHandler)
        return;

    switch (ev.type) {
    case TransportEvent::CONNECTED:
        mHandler->onConnected();
        break;
    case TransportEvent::DISCONNECTED:
        mHandler->onDisconnected();
        break;
    case TransportEvent::DATA:
        mHandler->onRecv(ev.data.data(), ev.data.size());
        break;
    case TransportEvent::ERROR:
        mHandler->onError(ev.error);
        break;
    default:
        break;
    }
}
