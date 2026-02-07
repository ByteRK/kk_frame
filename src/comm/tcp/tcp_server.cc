/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:29:26
 * @LastEditTime: 2026-02-07 10:48:46
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

int TcpServerTransport::createListenSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 8) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

bool TcpServerTransport::start() {
    if (mRunning)
        return false;

    mListenSock = createListenSocket();
    if (mListenSock < 0)
        return false;

    mRunning = true;
    mThread = std::thread(&TcpServerTransport::threadLoop, this);
    return true;
}

void TcpServerTransport::stop() {
    if (!mRunning)
        return;

    mRunning = false;

    if (mListenSock >= 0) {
        shutdown(mListenSock, SHUT_RDWR);
        close(mListenSock);
        mListenSock = -1;
    }

    for (auto& c : mClients) {
        shutdown(c.second, SHUT_RDWR);
        close(c.second);
    }
    mClients.clear();

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

        int ret = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (ret <= 0)
            continue;

        if (FD_ISSET(mListenSock, &rfds)) {
            int fd = accept(mListenSock, nullptr, nullptr);
            if (fd >= 0) {
                int cid = mNextClientId++;
                mClients[cid] = fd;

                TransportEvent ev;
                ev.type = TransportEvent::CLIENT_CONNECTED;
                ev.clientId = cid;
                postEvent(ev);
            }
        }

        uint8_t buf[1024];
        for (auto it = mClients.begin(); it != mClients.end();) {
            int cid = it->first;
            int fd = it->second;

            if (FD_ISSET(fd, &rfds)) {
                ssize_t n = recv(fd, buf, sizeof(buf), 0);
                if (n <= 0) {
                    TransportEvent ev;
                    ev.type = TransportEvent::CLIENT_DISCONNECTED;
                    ev.clientId = cid;
                    postEvent(ev);

                    close(fd);
                    it = mClients.erase(it);
                    continue;
                }

                TransportEvent ev;
                ev.type = TransportEvent::DATA;
                ev.clientId = cid;
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
        mHandler->onConnected(ev.clientId);
        break;
    case TransportEvent::CLIENT_DISCONNECTED:
        mHandler->onDisconnected(ev.clientId);
        break;
    case TransportEvent::DATA:
        mHandler->onRecv(ev.clientId,
            ev.data.data(),
            ev.data.size());
        break;
    default:
        break;
    }
}