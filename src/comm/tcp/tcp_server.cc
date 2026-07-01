/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.cc
 * @Description: Raw TCP server
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "tcp_server.h"

#include <cdlog.h>

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

TcpServer::TcpServer(uint16_t port)
    : mListenSock(-1),
      mNextClientId(1),
      mRunning(false),
      mHandler(nullptr) {
    mConfig.port = port;
    mConfig.backlog = 8;
    mConfig.readBufferSize = 4096;
    mConfig.sendTimeoutMs = 1000;
}

TcpServer::TcpServer(const Config& config)
    : mConfig(config),
      mListenSock(-1),
      mNextClientId(1),
      mRunning(false),
      mHandler(nullptr) {
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

int TcpServer::init() {
    if (mConfig.port == 0) {
        LOGE("TcpServer init failed. port is zero");
        return -1;
    }
    if (mConfig.sendTimeoutMs <= 0) {
        LOGE("TcpServer init failed. sendTimeoutMs=%d", mConfig.sendTimeoutMs);
        return -2;
    }
    return initEventDispatcher();
}

bool TcpServer::start() {
    if (mRunning.load()) {
        return true;
    }

    if (!isEventDispatcherReady() && init() != 0) {
        return false;
    }

    int listenSock = createListenSocket();
    if (listenSock < 0) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mSocketLock);
        mListenSock = listenSock;
    }

    mRunning.store(true);
    mThread = std::thread(&TcpServer::threadLoop, this);

    Event ev;
    ev.type = Event::CONNECTED;
    postEvent(ev);
    return true;
}

void TcpServer::stop() {
    if (!mRunning.load()) {
        shutdownEventDispatcher();
        return;
    }

    mRunning.store(false);
    closeAllSockets();

    if (mThread.joinable()) {
        mThread.join();
    }

    Event ev;
    ev.type = Event::DISCONNECTED;
    postEvent(ev);

    shutdownEventDispatcher();
}

bool TcpServer::isConnected() const {
    return mRunning.load();
}

ssize_t TcpServer::send(const uint8_t* data, size_t len, int id) {
    if (data == nullptr || len == 0 || id < 0) {
        return -1;
    }

    std::lock_guard<std::mutex> sendLock(mSendLock);
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(mSocketLock);
        auto it = mClients.find(id);
        if (it != mClients.end()) {
            fd = it->second;
        }
    }

    if (fd < 0) {
        return -1;
    }

    return sendAll(fd, data, len);
}

void TcpServer::dispatchEvent(const Event& ev) {
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
    case Event::CLIENT_CONNECTED:
        mHandler->onConnected(ev.id);
        break;
    case Event::CLIENT_DISCONNECTED:
        mHandler->onDisconnected(ev.id);
        break;
    case Event::DATA:
        if (!ev.data.empty()) {
            mHandler->onRecv(ev.data.data(), ev.data.size(), ev.id);
        }
        break;
    case Event::ERROR:
        mHandler->onError(ev.error);
        break;
    default:
        break;
    }
}

void TcpServer::threadLoop() {
    while (mRunning.load()) {
        fd_set readFds;
        FD_ZERO(&readFds);

        int maxFd = -1;
        std::map<int, int> clients;
        int listenSock = -1;
        {
            std::lock_guard<std::mutex> lock(mSocketLock);
            listenSock = mListenSock;
            clients = mClients;
        }

        if (listenSock < 0) {
            break;
        }

        FD_SET(listenSock, &readFds);
        maxFd = listenSock;
        for (const auto& client : clients) {
            FD_SET(client.second, &readFds);
            maxFd = std::max(maxFd, client.second);
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500 * 1000;
        int ready = select(maxFd + 1, &readFds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            Event ev;
            ev.type = Event::ERROR;
            ev.error = errno;
            postEvent(ev);
            continue;
        }
        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(listenSock, &readFds)) {
            int fd = accept(listenSock, nullptr, nullptr);
            if (fd >= 0) {
                int clientId = 0;
                {
                    std::lock_guard<std::mutex> lock(mSocketLock);
                    clientId = mNextClientId++;
                    mClients[clientId] = fd;
                }

                Event ev;
                ev.type = Event::CLIENT_CONNECTED;
                ev.id = clientId;
                postEvent(ev);
            }
        }

        std::vector<uint8_t> buffer(mConfig.readBufferSize > 0 ? mConfig.readBufferSize : 4096);
        for (const auto& client : clients) {
            const int clientId = client.first;
            const int fd = client.second;
            if (!FD_ISSET(fd, &readFds)) {
                continue;
            }

            ssize_t n = recv(fd, buffer.data(), buffer.size(), 0);
            if (n > 0) {
                Event ev;
                ev.type = Event::DATA;
                ev.id = clientId;
                ev.data.assign(buffer.begin(), buffer.begin() + n);
                postEvent(ev);
                continue;
            }

            if (n < 0 && errno == EINTR) {
                continue;
            }
            if (n < 0) {
                Event ev;
                ev.type = Event::ERROR;
                ev.id = clientId;
                ev.error = errno;
                postEvent(ev);
            }

            {
                std::lock_guard<std::mutex> sendLock(mSendLock);
                std::lock_guard<std::mutex> lock(mSocketLock);
                auto it = mClients.find(clientId);
                if (it != mClients.end()) {
                    shutdown(it->second, SHUT_RDWR);
                    close(it->second);
                    mClients.erase(it);
                }
            }

            Event ev;
            ev.type = Event::CLIENT_DISCONNECTED;
            ev.id = clientId;
            postEvent(ev);
        }
    }
}

int TcpServer::createListenSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("TcpServer socket failed. errno=%d err=%s", errno, strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mConfig.port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        LOGE("TcpServer bind failed. port=%u errno=%d err=%s",
            mConfig.port, errno, strerror(errno));
        close(fd);
        return -2;
    }

    if (listen(fd, mConfig.backlog) != 0) {
        LOGE("TcpServer listen failed. errno=%d err=%s", errno, strerror(errno));
        close(fd);
        return -3;
    }

    return fd;
}

void TcpServer::closeAllSockets() {
    std::lock_guard<std::mutex> sendLock(mSendLock);
    std::map<int, int> clients;
    int listenSock = -1;
    {
        std::lock_guard<std::mutex> lock(mSocketLock);
        listenSock = mListenSock;
        mListenSock = -1;
        clients.swap(mClients);
    }

    if (listenSock >= 0) {
        shutdown(listenSock, SHUT_RDWR);
        close(listenSock);
    }

    for (const auto& client : clients) {
        shutdown(client.second, SHUT_RDWR);
        close(client.second);
    }
}

ssize_t TcpServer::sendAll(int fd, const uint8_t* data, size_t len) {
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(mConfig.sendTimeoutMs);
    size_t written = 0;
    while (written < len) {
        if (std::chrono::steady_clock::now() >= deadline) {
            LOGE("TcpServer send timeout. written=%zu total=%zu", written, len);
            break;
        }

        ssize_t n = ::send(fd, data + written, len - written,
            MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) {
            written += static_cast<size_t>(n);
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
            if (remaining <= 0) {
                LOGE("TcpServer send timeout. written=%zu total=%zu", written, len);
                break;
            }

            pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            const int ready = poll(&pfd, 1, static_cast<int>(remaining));
            if (ready > 0 && (pfd.revents & POLLOUT) != 0) {
                continue;
            }
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready == 0) {
                LOGE("TcpServer send timeout. written=%zu total=%zu", written, len);
            }
        }
        break;
    }
    return static_cast<ssize_t>(written);
}
