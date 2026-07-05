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

#include <arpa/inet.h>
#include <chrono>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

TcpServer::TcpServer(uint16_t port)
    : mListenSock(-1),
    mNextClientId(1),
    mRunning(false) {
    mConfig.port = port;
    mConfig.backlog = 8;
    mConfig.readBufferSize = 4096;
    mConfig.sendTimeoutMs = 1000;
}

TcpServer::TcpServer(const Config& config)
    : mConfig(config),
    mListenSock(-1),
    mNextClientId(1),
    mRunning(false) { }

TcpServer::~TcpServer() {
    stop();
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
    return initAsyncDispatcher();
}

bool TcpServer::start() {
    if (mRunning.load()) {
        return true;
    }

    if (!isAsyncDispatcherReady() && init() != 0) {
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
        shutdownAsyncDispatcher();
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
    flushAsyncEvents();

    shutdownAsyncDispatcher();
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

void TcpServer::threadLoop() {
    while (mRunning.load()) {
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

        std::vector<pollfd> pollFds;
        std::vector<int> clientIds;
        pollFds.reserve(clients.size() + 1);
        clientIds.reserve(clients.size());

        pollfd listenPollFd;
        listenPollFd.fd = listenSock;
        listenPollFd.events = POLLIN;
        listenPollFd.revents = 0;
        pollFds.push_back(listenPollFd);

        for (const auto& client : clients) {
            pollfd clientPollFd;
            clientPollFd.fd = client.second;
            clientPollFd.events = POLLIN;
            clientPollFd.revents = 0;
            pollFds.push_back(clientPollFd);
            clientIds.push_back(client.first);
        }

        const int ready = poll(pollFds.data(), pollFds.size(), 500);
        if (!mRunning.load()) {
            break;
        }
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

        if ((pollFds.front().revents & POLLIN) != 0) {
            int fd = accept(listenSock, nullptr, nullptr);
            if (fd >= 0) {
                int clientId = 0;
                {
                    std::lock_guard<std::mutex> lock(mSocketLock);
                    clientId = mNextClientId++;
                    mClients[clientId] = fd;
                }

                Event ev;
                ev.type = Event::CONNECTED;
                ev.id = clientId;
                postEvent(ev);
            }
        }

        std::vector<uint8_t> buffer(mConfig.readBufferSize > 0 ? mConfig.readBufferSize : 4096);
        for (size_t i = 0; i < clientIds.size(); ++i) {
            const int clientId = clientIds[i];
            const int fd = pollFds[i + 1].fd;
            const short revents = pollFds[i + 1].revents;
            if ((revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) == 0) {
                continue;
            }

            ssize_t n = 0;
            int socketError = 0;
            if ((revents & POLLIN) != 0) {
                n = recv(fd, buffer.data(), buffer.size(), 0);
                if (n < 0) {
                    socketError = errno;
                }
            } else if ((revents & POLLNVAL) != 0) {
                socketError = EBADF;
            } else if ((revents & POLLERR) != 0) {
                socklen_t errorLen = sizeof(socketError);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &errorLen) != 0) {
                    socketError = errno;
                }
            }

            if (n > 0) {
                Event ev;
                ev.type = Event::DATA;
                ev.id = clientId;
                ev.data.assign(buffer.begin(), buffer.begin() + n);
                postEvent(ev);
                continue;
            }

            if (n < 0 && socketError == EINTR) {
                continue;
            }
            if (socketError != 0) {
                Event ev;
                ev.type = Event::ERROR;
                ev.id = clientId;
                ev.error = socketError;
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
            ev.type = Event::DISCONNECTED;
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
