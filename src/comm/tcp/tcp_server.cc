/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:29:26
 * @LastEditTime: 2026-02-07 14:08:09
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.cc
 * @Description: TCP服务端实现
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

TcpServer::TcpServer(uint16_t port)
    : mPort(port) {
    LOGI("TcpServer::TcpServer(%d)", port);
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

void TcpServer::init() {
    cdroid::App::getInstance().addEventHandler(this);
}

int TcpServer::createListenSocket() {
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

bool TcpServer::start() {
    if (mRunning)
        return false;

    mListenSock = createListenSocket();
    if (mListenSock < 0)
        return false;

    mRunning = true;
    mThread = std::thread(&TcpServer::threadLoop, this);
    return true;
}

void TcpServer::stop() {
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

void TcpServer::threadLoop() {
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

                Transport::Event ev;
                ev.type = Transport::Event::CLIENT_CONNECTED;
                ev.id = cid;
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
                    Transport::Event ev;
                    ev.type = Transport::Event::CLIENT_DISCONNECTED;
                    ev.id = cid;
                    postEvent(ev);

                    close(fd);
                    it = mClients.erase(it);
                    continue;
                }

                Transport::Event ev;
                ev.type = Transport::Event::DATA;
                ev.id = cid;
                ev.data.assign(buf, buf + n);
                postEvent(ev);
            }
            ++it;
        }
    }
}

ssize_t TcpServer::send(const uint8_t* data, size_t len, int id) {
    auto it = mClients.find(id);
    if (it == mClients.end())
        return -1;
    return ::send(it->second, data, len, 0);
}

void TcpServer::dispatchEvent(const Transport::Event& ev) {
    if (!mHandler)
        return;

    switch (ev.type) {
    case Transport::Event::CLIENT_CONNECTED:
        mHandler->onConnected(ev.id);
        break;
    case Transport::Event::CLIENT_DISCONNECTED:
        mHandler->onDisconnected(ev.id);
        break;
    case Transport::Event::DATA:
        mHandler->onRecv(
            ev.data.data(),
            ev.data.size(),
            ev.id);
        break;
    default:
        break;
    }
}