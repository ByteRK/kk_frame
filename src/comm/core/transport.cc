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

constexpr size_t Transport::DEFAULT_MAX_PENDING_EVENT_COUNT;

Transport::Transport()
    : mMaxPendingEventCount(DEFAULT_MAX_PENDING_EVENT_COUNT),
      mDroppedEventCount(0),
      mDispatchGeneration(0),
      mMainLooper(nullptr),
      mWakeFd(-1) {
}

Transport::~Transport() {
    shutdownEventDispatcher();
}

int Transport::initEventDispatcher(cdroid::Looper* mainLooper) {
    if (mWakeFd >= 0) {
        return 0;
    }

    mMainLooper = mainLooper != nullptr ? mainLooper : cdroid::Looper::getForThread();
    if (mMainLooper == nullptr) {
        LOGE("Transport init failed: main looper is null");
        return -1;
    }

    mWakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (mWakeFd < 0) {
        LOGE("Transport eventfd failed. errno=%d err=%s", errno, strerror(errno));
        return -2;
    }

    const int addResult = mMainLooper->addFd(
        mWakeFd, 0, cdroid::Looper::EVENT_INPUT, onWake, this);
    if (addResult < 0) {
        LOGE("Transport addFd failed. fd=%d", mWakeFd);
        close(mWakeFd);
        mWakeFd = -1;
        mMainLooper = nullptr;
        return -3;
    }

    return 0;
}

void Transport::shutdownEventDispatcher() {
    if (mMainLooper != nullptr && mWakeFd >= 0) {
        mMainLooper->removeFd(mWakeFd);
    }

    if (mWakeFd >= 0) {
        close(mWakeFd);
        mWakeFd = -1;
    }

    mMainLooper = nullptr;

    cancelEventDispatch();
}

void Transport::flushEventDispatch() {
    handleEvents();
}

bool Transport::isEventDispatcherReady() const {
    return mWakeFd >= 0 && mMainLooper != nullptr;
}

int Transport::checkEvents() {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mEvents.empty() ? 0 : 1;
}

int Transport::handleEvents() {
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

    return 1;
}

void Transport::cancelEventDispatch() {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        ++mDispatchGeneration;
        std::queue<Event> empty;
        mEvents.swap(empty);
    }
    mEventSpace.notify_all();
}

void Transport::setMaxPendingEventCount(size_t count) {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        mMaxPendingEventCount = count > 0 ? count : DEFAULT_MAX_PENDING_EVENT_COUNT;
    }
    mEventSpace.notify_all();
}

size_t Transport::maxPendingEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mMaxPendingEventCount;
}

size_t Transport::droppedEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mDroppedEventCount;
}

void Transport::postEvent(const Event& ev) {
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
            LOGE("Transport event queue full. dropped=%zu limit=%zu type=%d id=%d",
                droppedCount, queueLimit, static_cast<int>(ev.type), ev.id);
        }
        return;
    }

    wakeMainThread();
}

bool Transport::sendEvent(const Event& ev) {
    const uint64_t generation = mDispatchGeneration.load();
    dispatchEvent(ev);
    return mDispatchGeneration.load() == generation;
}

int Transport::onWake(int fd, int /*events*/, void* context) {
    Transport* transport = static_cast<Transport*>(context);
    if (transport == nullptr) {
        return 0;
    }

    transport->drainWakeFd();
    transport->handleEvents();
    return 1;
}

void Transport::wakeMainThread() {
    if (mWakeFd < 0) {
        return;
    }

    const uint64_t value = 1;
    const ssize_t rc = write(mWakeFd, &value, sizeof(value));
    if (rc < 0 && errno != EAGAIN) {
        LOGE("Transport wake failed. errno=%d err=%s", errno, strerror(errno));
    }
}

void Transport::drainWakeFd() {
    if (mWakeFd < 0) {
        return;
    }

    uint64_t value = 0;
    while (read(mWakeFd, &value, sizeof(value)) > 0) {
    }
}
