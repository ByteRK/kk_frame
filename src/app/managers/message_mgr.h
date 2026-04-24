/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-02 17:38:07
 * @LastEditTime: 2026-04-24 15:04:48
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

#include <cdlog.h>
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

#define g_msg MessageManager::instance()
#define MESSAGE_DEAL_INTERVAL 100

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
    void post(int msgType, int msgValue = 0, void* msgPtr = nullptr, int64_t delayMs = 0);

    // 跨线程发送带所有权的数据，消息处理完成后自动释放
    template <typename T>
    void postOwned(int msgType, int msgValue, std::unique_ptr<T> msgPtr, int64_t delayMs = 0);

    // 清除所有消息
    size_t clear();

    // 清除某个消息类型的所有消息
    size_t clear(int msgType);

protected:
    int checkEvents() override;
    int handleEvents() override;

private:
    struct Message {
        int     type{ 0 };
        int     value{ 0 };
        void*   ptr{ nullptr };
        int64_t dispatchTimeMs{ 0 };
        std::shared_ptr<void> owner;
        Message() { }
    };

    void dispatchMessage(const Message& msg);
    void enqueueMessage(Message&& msg);

private:
    /* 处理间隔 */
    int64_t mNextEventTimeMs{ 0 };

    /* 监听表 */
    std::mutex mListenerMutex;
    std::unordered_map<int, std::vector<MessageListener*> > mListeners;

    /* 消息队列 */
    std::mutex mQueueMutex;
    std::deque<Message> mQueue;
};

template <typename T>
void MessageManager::postOwned(int msgType, int msgValue, std::unique_ptr<T> msgPtr, int64_t delayMs) {
    FATAL_IF(!msgPtr, "[MessageManager] postOwned() got null payload, msgType=%d", msgType);
    FATAL_IF(delayMs < 0, "[MessageManager] postOwned() got negative delay, msgType=%d, delay=%lld",
        msgType, delayMs);

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