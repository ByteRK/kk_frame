/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-02 17:38:07
 * @LastEditTime: 2026-04-03 09:16:47
 * @FilePath: /kk_frame/src/app/managers/message_mgr.h
 * @Description: 消息分发器
 *
 *      // 跨线程分发数据指针用法
 *      std::unique_ptr<MyData> data(new MyData());
 *      data->a = 10; data->b = 20;
 *      MessageManager::instance()->postOwned(MSG_A, 0, std::move(data));
 *
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __MESSAGE_MGR_H__
#define __MESSAGE_MGR_H__

#include "template/singleton.h"

#include <core/looper.h>

#include <stdint.h>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <cstdlib>

// 是否启用线程检查
#ifndef MSG_MGR_ENABLE_THREAD_CHECK
#define MSG_MGR_ENABLE_THREAD_CHECK 0
#endif

#define MSG_MGR_FATAL(fmt, ...)                                                 \
    do {                                                                        \
        std::fprintf(stderr, "[MessageManager][FATAL] %s:%d: " fmt "\n",        \
                     __FILE__, __LINE__, ##__VA_ARGS__);                        \
        std::fflush(stderr);                                                    \
        std::abort();                                                           \
    } while (0)

#define MSG_MGR_FATAL_IF(cond, fmt, ...)                                        \
    do {                                                                        \
        if (cond) {                                                             \
            MSG_MGR_FATAL(fmt, ##__VA_ARGS__);                                  \
        }                                                                       \
    } while (0)

#if MSG_MGR_ENABLE_THREAD_CHECK
#define MSG_MGR_CHECK_MAIN_THREAD()   do { this->ensureMainThread(); } while (0)
#define MSG_MGR_CHECK_WORKER_THREAD() do { this->ensureWorkerThread(); } while (0)
#define MSG_MGR_CHECK_THREAD_BOUND()  do { this->ensureMainThreadBound(); } while (0)
#else
#define MSG_MGR_CHECK_MAIN_THREAD()   do {} while (0)
#define MSG_MGR_CHECK_WORKER_THREAD() do {} while (0)
#define MSG_MGR_CHECK_THREAD_BOUND()  do {} while (0)
#endif


#define g_msg MessageManager::instance()

class MessageListener {
public:
    virtual ~MessageListener() { }
    virtual void onMessage(int msgType, int msgValue = 0, void* msgPtr = nullptr) = 0;
};

class MessageManager : public cdroid::EventHandler,
    public Singleton<MessageManager> {
    friend Singleton<MessageManager>;

protected:
    MessageManager();
    ~MessageManager();

    static int64_t nowMs();

public:

    // 必须在主线程初始化
    void init();

    // 注册/反注册监听
    void add(int msgType, MessageListener* listener);
    void remove(int msgType, MessageListener* listener);
    void removeAll(MessageListener* listener);

    // 主线程同步发送，立即分发
    void send(int msgType, int msgValue = 0, void* msgPtr = nullptr);

    // 其它线程异步发送，支持延时分发
    void post(int msgType,
        int msgValue = 0,
        void* msgPtr = nullptr,
        int64_t delayMs = 0);

    // 跨线程发送带所有权的数据，消息处理完成后自动释放
    template <typename T>
    void postOwned(int msgType,
        int msgValue,
        std::unique_ptr<T> msgPtr,
        int64_t delayMs = 0);

    // 清除某个消息类型的所有排队消息（仅清队列，不影响正在处理的消息）
    // 返回清除数量；线程安全，可任意线程调用
    size_t clear(int msgType);

protected:
    int checkEvents() override;
    int handleEvents() override;

private:
    struct Message {
        int type{ 0 };
        int value{ 0 };
        void* ptr{ nullptr };
        int64_t dispatchTimeMs{ 0 };
        std::shared_ptr<void> owner;
        Message() { }
    };

    void ensureMainThreadBound() const;
    void ensureMainThread() const;
    void ensureWorkerThread() const;

    void dispatchMessage(const Message& msg);
    void enqueueMessage(Message&& msg);

private:
    mutable std::mutex mStateMutex;

    /* 线程判断 */
    bool            mMainThreadBound;
    std::thread::id mMainThreadId;

    /* 监听表 */
    std::mutex mListenerMutex;
    std::unordered_map<int, std::vector<MessageListener*> > mListeners;

    /* 消息队列 */
    std::mutex mQueueMutex;
    std::deque<Message> mQueue;
};

template <typename T>
void MessageManager::postOwned(int msgType,
    int msgValue,
    std::unique_ptr<T> msgPtr,
    int64_t delayMs) {
    MSG_MGR_CHECK_WORKER_THREAD();
    MSG_MGR_FATAL_IF(!msgPtr, "postOwned() got null payload, msgType=%d", msgType);
    MSG_MGR_FATAL_IF(delayMs < 0, "postOwned() got negative delay, msgType=%d, delay=%lld",
        msgType, (long long)delayMs);

    Message msg;
    msg.type = msgType;
    msg.value = msgValue;
    msg.ptr = msgPtr.get();
    msg.dispatchTimeMs = nowMs() + delayMs;
    msg.owner = std::shared_ptr<void>(
        msgPtr.release(),
        [](void* p) {
        delete static_cast<T*>(p);
    });

    enqueueMessage(std::move(msg));
}

#endif // __MESSAGE_MGR_H__