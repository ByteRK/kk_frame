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
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {

    constexpr int RESOLVE_WAIT_SLICE_MS = 50;
    constexpr int CONNECT_POLL_SLICE_MS = 100;

    struct ResolveState {
        std::mutex lock;
        std::condition_variable condition;
        bool done{ false };
        bool abandoned{ false };
        int error{ 0 };
        addrinfo* result{ nullptr };
    };

}

TcpClient::TcpClient(const std::string& host, uint16_t port)
    : mSock(-1),
    mRunning(false),
    mConnected(false) {
    mConfig.host = host;
    mConfig.port = port;
    mConfig.reconnectDelayMs = 2000;
    mConfig.readBufferSize = 4096;
    mConfig.sendTimeoutMs = 1000;
}

TcpClient::TcpClient(const Config& config)
    : mConfig(config),
    mSock(-1),
    mRunning(false),
    mConnected(false) { }

TcpClient::~TcpClient() {
    stop();
}

int TcpClient::init() {
    if (mConfig.host.empty() || mConfig.port == 0) {
        LOGE("TcpClient init failed. host=%s port=%u",
            mConfig.host.c_str(), mConfig.port);
        return -1;
    }
    if (mConfig.reconnectDelayMs < 0) {
        LOGE("TcpClient init failed. reconnectDelayMs=%d", mConfig.reconnectDelayMs);
        return -2;
    }
    if (mConfig.sendTimeoutMs <= 0) {
        LOGE("TcpClient init failed. sendTimeoutMs=%d", mConfig.sendTimeoutMs);
        return -3;
    }
    return initAsyncDispatcher();
}

bool TcpClient::start() {
    if (mRunning.load()) {
        return true;
    }

    if (!isAsyncDispatcherReady() && init() != 0) {
        return false;
    }

    mRunning.store(true);
    mThread = std::thread(&TcpClient::threadLoop, this);
    return true;
}

void TcpClient::stop() {
    if (!mRunning.load() && !mConnected.load()) {
        shutdownAsyncDispatcher();
        return;
    }

    mRunning.store(false);
    mStopCondition.notify_all();
    closeSocket();

    if (mThread.joinable()) {
        mThread.join();
    }

    if (mConnected.exchange(false)) {
        Event ev;
        ev.type = Event::DISCONNECTED;
        postEvent(ev);
        flushAsyncEvents();
    }

    shutdownAsyncDispatcher();
}

bool TcpClient::isConnected() const {
    return mConnected.load();
}

ssize_t TcpClient::send(const uint8_t* data, size_t len, int /*id*/) {
    if (data == nullptr || len == 0) {
        return -1;
    }

    std::lock_guard<std::mutex> sendLock(mSendLock);
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

void TcpClient::threadLoop() {
    while (mRunning.load()) {
        const int fd = connectServer();
        if (fd < 0) {
            waitForReconnectDelay();
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
            waitForReconnectDelay();
        }
    }
}

int TcpClient::connectServer() {
    char portText[16];
    snprintf(portText, sizeof(portText), "%u", mConfig.port);

    const std::shared_ptr<ResolveState> resolveState = std::make_shared<ResolveState>();
    const std::string host = mConfig.host;
    const std::string service = portText;
    try {
        std::thread resolver([resolveState, host, service]() {
            addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            addrinfo* resolved = nullptr;
            const int error = getaddrinfo(host.c_str(), service.c_str(), &hints, &resolved);
            bool abandoned = false;
            {
                std::lock_guard<std::mutex> lock(resolveState->lock);
                abandoned = resolveState->abandoned;
                if (!abandoned) {
                    resolveState->error = error;
                    resolveState->result = resolved;
                    resolveState->done = true;
                }
            }

            if (abandoned && resolved != nullptr) {
                freeaddrinfo(resolved);
            }
            resolveState->condition.notify_one();
        });
        resolver.detach();
    }
    catch (const std::system_error& e) {
        LOGE("TcpClient resolver thread failed. err=%s", e.what());
        return -1;
    }

    addrinfo* result = nullptr;
    int resolveError = 0;
    {
        std::unique_lock<std::mutex> lock(resolveState->lock);
        while (!resolveState->done && mRunning.load()) {
            resolveState->condition.wait_for(
                lock, std::chrono::milliseconds(RESOLVE_WAIT_SLICE_MS));
        }

        if (!resolveState->done) {
            resolveState->abandoned = true;
            return -1;
        }

        resolveError = resolveState->error;
        result = resolveState->result;
        resolveState->result = nullptr;
    }

    if (resolveError != 0) {
        LOGE("TcpClient getaddrinfo failed. host=%s port=%u err=%s",
            mConfig.host.c_str(), mConfig.port, gai_strerror(resolveError));
        if (result != nullptr) {
            freeaddrinfo(result);
        }
        return -1;
    }
    if (result == nullptr) {
        LOGE("TcpClient getaddrinfo returned no address. host=%s port=%u",
            mConfig.host.c_str(), mConfig.port);
        return -1;
    }

    int fd = -1;
    for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
        if (!mRunning.load()) {
            break;
        }

        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) {
            continue;
        }

        const int originalFlags = fcntl(fd, F_GETFL, 0);
        if (originalFlags < 0 || fcntl(fd, F_SETFL, originalFlags | O_NONBLOCK) != 0) {
            close(fd);
            fd = -1;
            continue;
        }

        bool connected = connect(fd, it->ai_addr, it->ai_addrlen) == 0;
        if (!connected && errno == EINPROGRESS) {
            while (mRunning.load()) {
                pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLOUT;
                pfd.revents = 0;

                const int ready = poll(&pfd, 1, CONNECT_POLL_SLICE_MS);
                if (ready < 0 && errno == EINTR) {
                    continue;
                }
                if (ready <= 0) {
                    if (ready < 0) {
                        break;
                    }
                    continue;
                }

                int socketError = 0;
                socklen_t errorLen = sizeof(socketError);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &errorLen) == 0 &&
                    socketError == 0) {
                    connected = true;
                }
                break;
            }
        }

        if (connected && mRunning.load() && fcntl(fd, F_SETFL, originalFlags) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    return fd;
}

void TcpClient::waitForReconnectDelay() {
    if (!mRunning.load() || mConfig.reconnectDelayMs == 0) {
        return;
    }

    std::unique_lock<std::mutex> lock(mStopLock);
    mStopCondition.wait_for(lock,
        std::chrono::milliseconds(mConfig.reconnectDelayMs),
        [this]() { return !mRunning.load(); });
}

void TcpClient::closeSocket() {
    std::lock_guard<std::mutex> sendLock(mSendLock);
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
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(mConfig.sendTimeoutMs);
    size_t written = 0;
    while (written < len) {
        if (std::chrono::steady_clock::now() >= deadline) {
            LOGE("TcpClient send timeout. written=%zu total=%zu", written, len);
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
                LOGE("TcpClient send timeout. written=%zu total=%zu", written, len);
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
                LOGE("TcpClient send timeout. written=%zu total=%zu", written, len);
            }
        }
        break;
    }
    return static_cast<ssize_t>(written);
}
