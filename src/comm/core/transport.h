/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:31:52
 * @LastEditTime: 2026-07-05 21:00:20
 * @FilePath: /kk_frame/src/comm/core/transport.h
 * @Description: 原始通讯抽象
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include <core/looper.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <sys/types.h>
#include <vector>

/// @brief 原始通讯事件回调
/// @note onRecv的数据生命周期仅限于本次回调
/// @note 禁止在回调中直接销毁 Transport、PacketChannel 或当前 handler
class TransportHandler {
public:
    virtual ~TransportHandler() { }
    virtual void onConnected(int id = -1) { }                                // 通讯通道建立事件，id 为客户端标识
    virtual void onDisconnected(int id = -1) { }                             // 通讯通道断开事件，id 为客户端标识
    virtual void onError(int err) { }                                        // 通讯错误事件，err 为错误码
    virtual void onRecv(const uint8_t* data, size_t len, int id = -1) = 0;   // 通讯数据事件（原始字节流）
};

/// @brief 原始通讯抽象层
/// @note 只负责通道生命周期、原始字节收发和事件分发
/// @note 不处理心跳、握手、校验或业务协议
/// @note 典型调用顺序为 setHandler() -> init() -> start()，结束时调用 stop()
class Transport {
protected:
    /// @brief 通讯标准事件
    struct Event {
        /// @brief 事件类型
        enum Type {
            CONNECTED,     // 连接成功
            DISCONNECTED,  // 连接断开
            DATA,          // 接收到数据
            ERROR,         // 发生错误
        } type;

        int id{ -1 };              // 客户端标识
        int error{ 0 };            // 错误码
        std::vector<uint8_t> data; // 原始数据
    };

private:
    TransportHandler* mHandler{ nullptr };    // 事件消费者

protected:
    Transport();
    virtual ~Transport();

public:
    /// @brief 校验配置并初始化资源
    /// @return 0 成功，非 0 失败
    virtual int init() = 0;

    /// @brief 启动通讯
    /// @return true 成功，false 失败
    /// @note 重复启动应保持幂等
    virtual bool start() = 0;

    /// @brief 停止通讯并释放运行资源
    /// @note 重复调用应保持幂等
    virtual void stop() = 0;

    /// @brief 返回当前通讯通道是否处于运行或连接状态
    /// @return true 处于运行或连接状态，false 未运行或未连接
    virtual bool isConnected() const = 0;

    /// @brief 发送原始数据
    /// @param data 待发送数据，调用期间必须有效
    /// @param len 数据长度，必须大于 0
    /// @param id 多客户端通讯下的目标客户端标识；点对点通道忽略该参数
    /// @return 实际发送字节数，参数或连接状态无效时返回 -1
    virtual ssize_t send(const uint8_t* data, size_t len, int id = -1) = 0;

    /// @brief 设置事件消费者
    /// @param handler 事件消费者
    void setHandler(TransportHandler* handler);

protected:
    /// @brief 事件分发
    /// @param ev 待分发事件
    /// @note 实现层需要保障线程已回归
    void dispatchEvent(const Event& ev);
};


/// @brief 异步通讯抽象层 [带FD跨线程分发能力]
/// @note 实现层调用 postEvent() 分发事件，事件由 initAsyncDispatcher() 绑定的 Looper 线程执行
class AsyncTransport : public Transport {
public:
    static constexpr size_t DEFAULT_MAX_PENDING_EVENT_COUNT = 256;  // 默认待分发事件数量上限

private:
    mutable std::mutex      mEventLock;                   // 事件队列互斥锁
    std::condition_variable mEventSpace;                  // 事件队列空闲检测
    std::queue<Event>       mEvents;                      // 事件队列
    size_t                  mMaxPendingEventCount;        // 待分发事件数量上限
    size_t                  mDroppedEventCount{ 0 };      // 因队列满载而丢弃的事件数量
    std::atomic<uint64_t>   mDispatchGeneration{ 0 };     // 事件分发批次标识
    cdroid::Looper*         mCallbackLooper{ nullptr };   // 事件分发目标 Looper
    int                     mWakeFd{ -1 };                // 事件分发唤醒 FD

protected:
    AsyncTransport();
    ~AsyncTransport() override;

public:
    void   setMaxPendingEventCount(size_t count);
    size_t maxPendingEventCount() const;
    size_t droppedEventCount() const;

protected:
    int    initAsyncDispatcher(cdroid::Looper* callbackLooper = nullptr);
    void   shutdownAsyncDispatcher();
    void   flushAsyncEvents();
    bool   isAsyncDispatcherReady() const;
    void   cancelAsyncEvents();

    void   postEvent(const Event& ev);

private:
    static int onWake(int fd, int events, void* context);
    void       wakeMainThread();
    void       drainWakeFd();
    void       dispatchPendingEvents();
};

#endif // !__TRANSPORT_H__
