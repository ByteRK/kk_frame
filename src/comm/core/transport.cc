/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/core/transport.cc
 * @Description: Raw transport base
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "transport.h"

#include <cdlog.h>

#include <chrono>
#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace {

    constexpr int EVENT_QUEUE_WAIT_TIMEOUT_MS = 100;

}

Transport::Transport() : mHandler(nullptr) { }

Transport::~Transport() { }

void Transport::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

void Transport::dispatchEvent(const Event& ev) {
    if (mHandler == nullptr) {
        return;
    }

    switch (ev.type) {
    case Event::CONNECTED:
        mHandler->onConnected(ev.id);
        break;
    case Event::DISCONNECTED:
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
    }
}

constexpr size_t AsyncTransport::DEFAULT_MAX_PENDING_EVENT_COUNT;

AsyncTransport::AsyncTransport()
    : mMaxPendingEventCount(DEFAULT_MAX_PENDING_EVENT_COUNT),
    mDroppedEventCount(0),
    mDispatchGeneration(0),
    mCallbackLooper(nullptr),
    mWakeFd(-1) { }

AsyncTransport::~AsyncTransport() {
    shutdownAsyncDispatcher();
}

int AsyncTransport::initAsyncDispatcher(cdroid::Looper* callbackLooper) {
    if (mWakeFd >= 0) {
        return 0;
    }

    mCallbackLooper = callbackLooper != nullptr ? callbackLooper : cdroid::Looper::getForThread();
    if (mCallbackLooper == nullptr) {
        LOGE("AsyncTransport init failed: callback looper is null");
        return -1;
    }

    mWakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (mWakeFd < 0) {
        LOGE("AsyncTransport eventfd failed. errno=%d err=%s", errno, strerror(errno));
        return -2;
    }

    const int addResult = mCallbackLooper->addFd(
        mWakeFd, 0, cdroid::Looper::EVENT_INPUT, onWake, this);
    if (addResult < 0) {
        LOGE("AsyncTransport addFd failed. fd=%d", mWakeFd);
        close(mWakeFd);
        mWakeFd = -1;
        mCallbackLooper = nullptr;
        return -3;
    }

    return 0;
}

void AsyncTransport::shutdownAsyncDispatcher() {
    if (mCallbackLooper != nullptr && mWakeFd >= 0) {
        mCallbackLooper->removeFd(mWakeFd);
    }

    if (mWakeFd >= 0) {
        close(mWakeFd);
        mWakeFd = -1;
    }

    mCallbackLooper = nullptr;

    cancelAsyncEvents();
}

void AsyncTransport::flushAsyncEvents() {
    dispatchPendingEvents();
}

bool AsyncTransport::isAsyncDispatcherReady() const {
    return mWakeFd >= 0 && mCallbackLooper != nullptr;
}

void AsyncTransport::dispatchPendingEvents() {
    const uint64_t generation = mDispatchGeneration.load();
    std::queue<Event> events;
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        mEvents.swap(events);
    }
    mEventSpace.notify_all();

    while (!events.empty()) {
        dispatchEvent(events.front());
        events.pop();
        if (mDispatchGeneration.load() != generation) {
            break;
        }
    }

}

void AsyncTransport::cancelAsyncEvents() {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        ++mDispatchGeneration;
        std::queue<Event> empty;
        mEvents.swap(empty);
    }
    mEventSpace.notify_all();
}

void AsyncTransport::setMaxPendingEventCount(size_t count) {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        mMaxPendingEventCount = count > 0 ? count : DEFAULT_MAX_PENDING_EVENT_COUNT;
    }
    mEventSpace.notify_all();
}

size_t AsyncTransport::maxPendingEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mMaxPendingEventCount;
}

size_t AsyncTransport::droppedEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mDroppedEventCount;
}

void AsyncTransport::postEvent(const Event& ev) {
    size_t droppedCount = 0;
    size_t queueLimit = 0;
    {
        std::unique_lock<std::mutex> lock(mEventLock);
        const bool hasSpace = mEventSpace.wait_for(lock,
            std::chrono::milliseconds(EVENT_QUEUE_WAIT_TIMEOUT_MS),
            [this]() { return mEvents.size() < mMaxPendingEventCount; });
        if (!hasSpace) {
            droppedCount = ++mDroppedEventCount;
            queueLimit = mMaxPendingEventCount;
        } else {
            mEvents.push(ev);
        }
    }

    if (droppedCount > 0) {
        if (droppedCount == 1 || (droppedCount & (droppedCount - 1)) == 0) {
            LOGE("AsyncTransport event queue full. dropped=%zu limit=%zu type=%d id=%d",
                droppedCount, queueLimit, static_cast<int>(ev.type), ev.id);
        }
        return;
    }

    wakeMainThread();
}

int AsyncTransport::onWake(int fd, int /*events*/, void* context) {
    AsyncTransport* transport = static_cast<AsyncTransport*>(context);
    if (transport == nullptr) {
        return 0;
    }

    transport->drainWakeFd();
    transport->dispatchPendingEvents();
    return 1;
}

void AsyncTransport::wakeMainThread() {
    if (mWakeFd < 0) {
        return;
    }

    const uint64_t value = 1;
    const ssize_t rc = write(mWakeFd, &value, sizeof(value));
    if (rc < 0 && errno != EAGAIN) {
        LOGE("AsyncTransport wake failed. errno=%d err=%s", errno, strerror(errno));
    }
}

void AsyncTransport::drainWakeFd() {
    if (mWakeFd < 0) {
        return;
    }

    uint64_t value = 0;
    while (read(mWakeFd, &value, sizeof(value)) > 0) {
    }
}
