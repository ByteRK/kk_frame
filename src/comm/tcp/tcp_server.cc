/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:29:26
 * @LastEditTime: 2026-02-05 17:49:54
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "tcp_server.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <core/app.h>
#include <cdlog.h>

TcpServerTransport::TcpServerTransport(uint16_t port)
    : mPort(port) {
    LOGI("TcpServerTransport::TcpServerTransport(%d)", port);
}

TcpServerTransport::~TcpServerTransport() {
    stop();
}

void TcpServerTransport::setHandler(ITcpServerHandler* handler) {
    mHandler = handler;
}

void TcpServerTransport::init() {
    cdroid::App::getInstance().addEventHandler(this);
}

bool TcpServerTransport::start() {
    mListenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenSock < 0)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(mListenSock, (sockaddr*)&addr, sizeof(addr)) < 0)
        return false;

    listen(mListenSock, 5);

    mRunning = true;
    mThread = std::thread(&TcpServerTransport::threadLoop, this);
    return true;
}

void TcpServerTransport::stop() {
    mRunning = false;
    if (mListenSock >= 0)
        close(mListenSock);
    if (mThread.joinable())
        mThread.join();
}

void TcpServerTransport::threadLoop() {
    fd_set rfds;

    while (mRunning) {
        FD_ZERO(&rfds);
        FD_SET(mListenSock, &rfds);
        int maxfd = mListenSock;

        for (auto& c : mClients) {
            FD_SET(c.second, &rfds);
            maxfd = std::max(maxfd, c.second);
        }

        if (select(maxfd + 1, &rfds, nullptr, nullptr, nullptr) <= 0)
            continue;

        if (FD_ISSET(mListenSock, &rfds)) {
            int fd = accept(mListenSock, nullptr, nullptr);
            int cid = mNextClientId++;
            mClients[cid] = fd;

            TransportEvent ev;
            ev.type = TransportEvent::CLIENT_CONNECTED;
            ev.clientId = cid;
            postEvent(ev);
        }

        uint8_t buf[1024];
        for (auto it = mClients.begin(); it != mClients.end();) {
            if (FD_ISSET(it->second, &rfds)) {
                ssize_t n = recv(it->second, buf, sizeof(buf), 0);
                if (n <= 0) {
                    TransportEvent ev;
                    ev.type = TransportEvent::CLIENT_DISCONNECTED;
                    ev.clientId = it->first;
                    postEvent(ev);

                    close(it->second);
                    it = mClients.erase(it);
                    continue;
                }

                TransportEvent ev;
                ev.type = TransportEvent::DATA;
                ev.clientId = it->first;
                ev.data.assign(buf, buf + n);
                postEvent(ev);
            }
            ++it;
        }
    }
}

ssize_t TcpServerTransport::send(const uint8_t* data, size_t len, int clientId) {
    auto it = mClients.find(clientId);
    if (it == mClients.end())
        return -1;
    return ::send(it->second, data, len, 0);
}

void TcpServerTransport::dispatchEvent(const TransportEvent& ev) {
    if (!mHandler)
        return;

    switch (ev.type) {
    case TransportEvent::CLIENT_CONNECTED:
        mHandler->onClientConnected(ev.clientId);
        break;
    case TransportEvent::CLIENT_DISCONNECTED:
        mHandler->onClientDisconnected(ev.clientId);
        break;
    case TransportEvent::DATA:
        mHandler->onRecv(ev.clientId, ev.data.data(), ev.data.size());
        break;
    default:
        break;
    }
}
