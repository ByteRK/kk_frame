/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.cc
 * @Description: Raw TCP client
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "tcp_client.h"

#include <cdlog.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

TcpClient::TcpClient(const std::string& host, uint16_t port)
    : mSock(-1),
      mRunning(false),
      mConnected(false),
      mHandler(nullptr) {
    mConfig.host = host;
    mConfig.port = port;
    mConfig.reconnectDelayMs = 2000;
    mConfig.readBufferSize = 4096;
}

TcpClient::TcpClient(const Config& config)
    : mConfig(config),
      mSock(-1),
      mRunning(false),
      mConnected(false),
      mHandler(nullptr) {
}

TcpClient::~TcpClient() {
    stop();
}

void TcpClient::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

int TcpClient::init() {
    if (mConfig.host.empty() || mConfig.port == 0) {
        LOGE("TcpClient init failed. host=%s port=%u",
            mConfig.host.c_str(), mConfig.port);
        return -1;
    }
    return initEventDispatcher();
}

bool TcpClient::start() {
    if (mRunning.load()) {
        return true;
    }

    if (!isEventDispatcherReady() && init() != 0) {
        return false;
    }

    mRunning.store(true);
    mThread = std::thread(&TcpClient::threadLoop, this);
    return true;
}

void TcpClient::stop() {
    if (!mRunning.load() && !mConnected.load()) {
        shutdownEventDispatcher();
        return;
    }

    mRunning.store(false);
    closeSocket();

    if (mThread.joinable()) {
        mThread.join();
    }

    if (mConnected.exchange(false)) {
        Event ev;
        ev.type = Event::DISCONNECTED;
        postEvent(ev);
    }

    shutdownEventDispatcher();
}

bool TcpClient::isConnected() const {
    return mConnected.load();
}

ssize_t TcpClient::send(const uint8_t* data, size_t len, int /*id*/) {
    if (data == nullptr || len == 0) {
        return -1;
    }

    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(mSocketLock);
        fd = mSock;
    }

    if (fd < 0 || !mConnected.load()) {
        return -1;
    }

    return sendAll(fd, data, len);
}

void TcpClient::dispatchEvent(const Event& ev) {
    if (mHandler == nullptr) {
        return;
    }

    switch (ev.type) {
    case Event::CONNECTED:
        mHandler->onConnected();
        break;
    case Event::DISCONNECTED:
        mHandler->onDisconnected();
        break;
    case Event::DATA:
        if (!ev.data.empty()) {
            mHandler->onRecv(ev.data.data(), ev.data.size());
        }
        break;
    case Event::ERROR:
        mHandler->onError(ev.error);
        break;
    default:
        break;
    }
}

void TcpClient::threadLoop() {
    while (mRunning.load()) {
        const int fd = connectServer();
        if (fd < 0) {
            poll(nullptr, 0, mConfig.reconnectDelayMs);
            continue;
        }

        mConnected.store(true);
        {
            std::lock_guard<std::mutex> lock(mSocketLock);
            mSock = fd;
        }

        Event connected;
        connected.type = Event::CONNECTED;
        postEvent(connected);

        std::vector<uint8_t> buffer(mConfig.readBufferSize > 0 ? mConfig.readBufferSize : 4096);
        while (mRunning.load()) {
            ssize_t n = recv(fd, buffer.data(), buffer.size(), 0);
            if (n > 0) {
                Event ev;
                ev.type = Event::DATA;
                ev.data.assign(buffer.begin(), buffer.begin() + n);
                postEvent(ev);
                continue;
            }

            if (n < 0 && errno == EINTR) {
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                poll(nullptr, 0, 10);
                continue;
            }

            if (n < 0) {
                Event ev;
                ev.type = Event::ERROR;
                ev.error = errno;
                postEvent(ev);
            }
            break;
        }

        closeSocket();
        if (mConnected.exchange(false)) {
            Event ev;
            ev.type = Event::DISCONNECTED;
            postEvent(ev);
        }

        if (mRunning.load()) {
            poll(nullptr, 0, mConfig.reconnectDelayMs);
        }
    }
}

int TcpClient::connectServer() {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char portText[16];
    snprintf(portText, sizeof(portText), "%u", mConfig.port);

    addrinfo* result = nullptr;
    int rc = getaddrinfo(mConfig.host.c_str(), portText, &hints, &result);
    if (rc != 0) {
        LOGE("TcpClient getaddrinfo failed. host=%s port=%u err=%s",
            mConfig.host.c_str(), mConfig.port, gai_strerror(rc));
        return -1;
    }

    int fd = -1;
    for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) {
            continue;
        }

        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    return fd;
}

void TcpClient::closeSocket() {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(mSocketLock);
        fd = mSock;
        mSock = -1;
    }

    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

ssize_t TcpClient::sendAll(int fd, const uint8_t* data, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = ::send(fd, data + written, len - written, MSG_NOSIGNAL);
        if (n > 0) {
            written += static_cast<size_t>(n);
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            poll(nullptr, 0, 10);
            continue;
        }
        break;
    }
    return static_cast<ssize_t>(written);
}
