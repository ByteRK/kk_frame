/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:31:52
 * @LastEditTime: 2026-07-05 21:00:07
 * @FilePath: /kk_frame/src/comm/core/transport.cc
 * @Description: 原始通讯抽象
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

constexpr int EVENT_QUEUE_WAIT_TIMEOUT_MS = 100; // 事件队列等待超时时间

/// @brief 原始通讯抽象层构造
Transport::Transport() { }

/// @brief 原始通讯抽象层析构
Transport::~Transport() { }

/// @brief 设置事件消费者
/// @param handler 事件消费者
void Transport::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

/// @brief 分发事件
/// @param ev 事件
void Transport::dispatchEvent(const Event& ev) {
    if (mHandler == nullptr) return;
    switch (ev.type) {
    case Event::CONNECTED: {
        mHandler->onConnected(ev.id);
    }   break;
    case Event::DISCONNECTED: {
        mHandler->onDisconnected(ev.id);
    }   break;
    case Event::DATA: {
        if (ev.data.empty())break;
        mHandler->onRecv(ev.data.data(), ev.data.size(), ev.id);
    }   break;
    case Event::ERROR: {
        mHandler->onError(ev.error);
    }   break;
    default: {
        LOGW("Unknown event type: %d", ev.type);
    }   break;
    }
}

/// @brief 异步通讯抽象层构造
AsyncTransport::AsyncTransport()
    : mMaxPendingEventCount(DEFAULT_MAX_PENDING_EVENT_COUNT) { }

/// @brief 异步通讯抽象层析构
AsyncTransport::~AsyncTransport() {
    shutdownAsyncDispatcher();
}

/// @brief 设置最大分发事件数量
/// @param count 最大分发事件数量
/// @note 如果count为0，则使用默认值DEFAULT_MAX_PENDING_EVENT_COUNT
void AsyncTransport::setMaxPendingEventCount(size_t count) {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        mMaxPendingEventCount = count > 0 ? count : DEFAULT_MAX_PENDING_EVENT_COUNT;
    }
    mEventSpace.notify_all();
}

/// @brief 返回最大分发事件数量
/// @return 最大分发事件数量
size_t AsyncTransport::maxPendingEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mMaxPendingEventCount;
}

/// @brief 返回丢弃的事件数量
/// @return 丢弃的事件数量
/// @note 仅记录因队列持续满载而丢弃的累计事件
size_t AsyncTransport::droppedEventCount() const {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mDroppedEventCount;
}

/// @brief 初始化跨线程事件分发器
/// @param callbackLooper 目标 Looper
/// @return 0 成功，非 0 失败
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

/// @brief 关闭跨线程事件分发器
/// @note 注销 eventfd、清空待处理事件并释放分发资源
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

/// @brief 立即分发所有待处理的事件
/// @note 用于关闭前保留生命周期回调
void AsyncTransport::flushAsyncEvents() {
    dispatchPendingEvents();
}

/// @brief 判断跨线程事件分发器是否可用
/// @return true 可用，false 不可用
bool AsyncTransport::isAsyncDispatcherReady() const {
    return mWakeFd >= 0 && mCallbackLooper != nullptr;
}

/// @brief 清空所有待处理的事件
void AsyncTransport::cancelAsyncEvents() {
    {
        std::lock_guard<std::mutex> lock(mEventLock);
        ++mDispatchGeneration;
        std::queue<Event> empty;
        mEvents.swap(empty);
    }
    mEventSpace.notify_all();
}

/// @brief 投递事件
/// @param ev 事件
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

/// @brief FD 触发唤醒
/// @param fd 来源FD
/// @param events 事件
/// @param context 上下文 [跨线程事件分发器指针]
/// @return 唤醒结果
int AsyncTransport::onWake(int fd, int events, void* context) {
    AsyncTransport* transport = static_cast<AsyncTransport*>(context);
    if (transport == nullptr) return 0;

    transport->drainWakeFd();
    transport->dispatchPendingEvents();
    return 1;
}

/// @brief 通过 FD 唤醒主线程
void AsyncTransport::wakeMainThread() {
    if (mWakeFd < 0) return;
    const uint64_t value = 1;
    const ssize_t rc = write(mWakeFd, &value, sizeof(value));
    if (rc < 0 && errno != EAGAIN) {
        LOGE("AsyncTransport wake failed. errno=%d err=%s", errno, strerror(errno));
    }
}

/// @brief 清空唤醒 FD
void AsyncTransport::drainWakeFd() {
    if (mWakeFd < 0) return;
    uint64_t value = 0;
    while (read(mWakeFd, &value, sizeof(value)) > 0) { }
}

/// @brief 分发事件
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
        // 批次变更，退出
        if (mDispatchGeneration.load() != generation) {
            break;
        }
    }
}
