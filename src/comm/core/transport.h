/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/core/transport.h
 * @Description: Raw transport base
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include "transport_handler.h"

#include <core/looper.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <sys/types.h>
#include <vector>

/**
 * @brief 原始通讯抽象层
 *
 * 本层只负责通道生命周期、原始字节收发和事件分发，不处理心跳、握手、校验或
 * 业务协议。典型调用顺序为 setHandler() -> init() -> start()，结束时调用 stop()。
 */
class Transport : public cdroid::EventHandler {
public:
    static constexpr size_t DEFAULT_MAX_PENDING_EVENT_COUNT = 256;

    /** @brief 跨线程投递时使用的内部事件载体。 */
    struct Event {
        /** @brief 通道级事件和 TCP 服务端客户端级事件。 */
        enum Type {
            CONNECTED,
            DISCONNECTED,
            DATA,
            ERROR,
            CLIENT_CONNECTED,
            CLIENT_DISCONNECTED,
        } type;

        int id{ -1 };
        int error{ 0 };
        std::vector<uint8_t> data;
    };

protected:
    Transport();

public:
    virtual ~Transport();

    /** @brief 设置事件接收者；对象不接管 handler 的生命周期。 */
    virtual void setHandler(TransportHandler* handler) = 0;
    /** @brief 校验配置并初始化资源，成功返回 0，失败返回非 0。 */
    virtual int init() = 0;
    /** @brief 启动通讯；重复启动应保持幂等，成功返回 true。 */
    virtual bool start() = 0;
    /** @brief 停止通讯并释放运行资源；允许重复调用。 */
    virtual void stop() = 0;
    /** @brief 返回当前通道是否可用于收发。 */
    virtual bool isConnected() const = 0;
    /**
     * @brief 发送原始字节。
     * @param data 待发送数据，调用期间必须有效。
     * @param len 数据长度，必须大于 0。
     * @param id TCP 服务端的目标客户端标识；点对点通道忽略该参数。
     * @return 实际发送字节数，参数或连接状态无效时返回 -1。
     */
    virtual ssize_t send(const uint8_t* data, size_t len, int id = -1) = 0;

    /** @brief Looper 查询是否存在待分发事件。 */
    int checkEvents() override;
    /** @brief 在 Looper 所在线程中取出并分发全部待处理事件。 */
    int handleEvents() override;

    /** @brief 设置待分发事件数量上限；传入 0 时恢复默认值。 */
    void setMaxPendingEventCount(size_t count);
    /** @brief 返回当前待分发事件数量上限。 */
    size_t maxPendingEventCount() const;
    /** @brief 返回因队列持续满载而丢弃的累计事件数量。 */
    size_t droppedEventCount() const;

protected:
    /**
     * @brief 初始化基于 eventfd 的跨线程事件分发器。
     * @param mainLooper 目标 Looper；为空时使用当前线程的 Looper。
     */
    int initEventDispatcher(cdroid::Looper* mainLooper = nullptr);
    /** @brief 注销 eventfd、清空待处理事件并释放分发资源。 */
    void shutdownEventDispatcher();
    /** @brief 立即分发当前已经排队的事件，用于关闭前保留生命周期回调。 */
    void flushEventDispatch();
    /** @brief 返回跨线程事件分发器是否已经可用。 */
    bool isEventDispatcherReady() const;
    /** @brief 使当前批次尚未分发的事件失效，并清空排队事件。 */
    void cancelEventDispatch();

    /** @brief 将事件加入队列，并唤醒 Looper 线程后异步分发。 */
    void postEvent(const Event& ev);
    /** @brief 在当前线程同步分发；回调触发取消或停止时返回 false。 */
    bool sendEvent(const Event& ev);
    /** @brief 由派生类将内部事件转换为 TransportHandler 回调。 */
    virtual void dispatchEvent(const Event& ev) = 0;

private:
    static int onWake(int fd, int events, void* context);
    void wakeMainThread();
    void drainWakeFd();

private:
    mutable std::mutex mEventLock;
    std::condition_variable mEventSpace;
    std::queue<Event> mEvents;
    size_t mMaxPendingEventCount;
    size_t mDroppedEventCount;
    std::atomic<uint64_t> mDispatchGeneration;
    cdroid::Looper* mMainLooper;
    int mWakeFd;
};

#endif // !__TRANSPORT_H__
