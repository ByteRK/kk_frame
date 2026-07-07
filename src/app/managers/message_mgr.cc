/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-02 17:41:09
 * @LastEditTime: 2026-07-07 10:37:07
 * @FilePath: /kk_frame/src/app/managers/message_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "message_mgr.h"
#include <core/systemclock.h>
#include <algorithm>

/// @brief 消息监听类析构
MessageListener::~MessageListener() {
#if 0 // 降低性能影响，应用层自行管控
    g_msg->removeAll(this);
#endif
}

/// @brief 消息监听变量析构
MessageListenerVariable::~MessageListenerVariable() {
#if 0 // 降低性能影响，应用层自行管控
    unregisterAll();
#endif
}

/// @brief 设置消息回调
void MessageListenerVariable::setCallBack(MessageCallBack cb) {
    mMsgCB = cb;
}

/// @brief 注册到指定消息类型
void MessageListenerVariable::registerTo(int msgType) {
    g_msg->add(msgType, this);
}

/// @brief 从指定消息类型注销
void MessageListenerVariable::unregisterFrom(int msgType) {
    g_msg->remove(msgType, this);
}

/// @brief 从所有消息类型注销
void MessageListenerVariable::unregisterAll() {
    g_msg->removeAll(this);
}

/// @brief 消息回调
void MessageListenerVariable::onMessage(int msgType, int msgValue, void* msgPtr) {
    if (mMsgCB) mMsgCB(msgType, msgValue, msgPtr);
}

MessageManager::MessageManager() { }

MessageManager::~MessageManager() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
    clear();
}

int64_t MessageManager::nowMs() {
    return cdroid::SystemClock::uptimeMillis();
}

void MessageManager::init() {
    cdroid::Looper::getMainLooper()->addEventHandler(this);
    LOGI("[MessageManager] init() Enjoy using it!!!");
}

void MessageManager::add(int msgType, MessageListener* listener) {
    FATAL_IF(listener == nullptr, "[MessageManager] add() got null listener, msgType=%d", msgType);

    std::lock_guard<std::mutex> lock(mListenerMutex);
    std::vector<MessageListener*>& listeners = mListeners[msgType];
    if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end()) {
        FATAL("[MessageManager] duplicate listener registration, msgType=%d", msgType);
    }

    listeners.push_back(listener);
}

void MessageManager::remove(int msgType, MessageListener* listener) {
    FATAL_IF(listener == nullptr, "[MessageManager] remove() got null listener, msgType=%d", msgType);

    std::lock_guard<std::mutex> lock(mListenerMutex);
    auto it = mListeners.find(msgType);
    if (it == mListeners.end()) return;

    std::vector<MessageListener*>& listeners = it->second;
    auto lit = std::find(listeners.begin(), listeners.end(), listener);
    if (lit == listeners.end()) return;

    listeners.erase(lit);
    if (listeners.empty()) mListeners.erase(it);
}

void MessageManager::removeAll(MessageListener* listener) {
    FATAL_IF(listener == nullptr, "[MessageManager] removeAll() got null listener");

    std::lock_guard<std::mutex> lock(mListenerMutex);

    for (auto it = mListeners.begin(); it != mListeners.end();) {
        std::vector<MessageListener*>& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());

        if (listeners.empty()) {
            it = mListeners.erase(it);
        } else {
            ++it;
        }
    }
}

void MessageManager::send(int msgType, int msgValue, void* msgPtr) {
    Message msg;
    msg.type = msgType;
    msg.value = msgValue;
    msg.ptr = msgPtr;
    msg.dispatchTimeMs = nowMs();

    dispatchMessage(msg);
}

void MessageManager::post(int msgType, int msgValue, void* msgPtr, int64_t delayMs) {
    FATAL_IF(delayMs < 0, "[MessageManager] post() got negative delay, msgType=%d, delay=%lld",
        msgType, (long long)delayMs);

    Message msg;
    msg.type = msgType;
    msg.value = msgValue;
    msg.ptr = msgPtr;
    msg.dispatchTimeMs = nowMs() + delayMs;

    enqueueMessage(std::move(msg));
}

void MessageManager::enqueueMessage(Message&& msg) {
    std::lock_guard<std::mutex> lock(mQueueMutex);

    auto pos = mQueue.end();
    for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
        if (msg.dispatchTimeMs < it->dispatchTimeMs) {
            pos = it;
            break;
        }
    }

    mQueue.insert(pos, std::move(msg));
}

size_t MessageManager::clear() {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    size_t oldSize = mQueue.size();
    mQueue.clear();
    return oldSize;
}

size_t MessageManager::clear(int msgType) {
    std::lock_guard<std::mutex> lock(mQueueMutex);

    size_t oldSize = mQueue.size();
    for (auto it = mQueue.begin(); it != mQueue.end();) {
        if (it->type == msgType) {
            it = mQueue.erase(it);
        } else {
            ++it;
        }
    }

    return oldSize - mQueue.size();
}

int MessageManager::checkEvents() {
    const int64_t now = nowMs();
    int readyCount = 0;

    if (now >= mNextEventTimeMs) {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
            if (it->dispatchTimeMs <= now) {
                ++readyCount;
            } else {
                break;
            }
        }
        mNextEventTimeMs = now + MESSAGE_DEAL_INTERVAL;
    }

    return readyCount;
}

int MessageManager::handleEvents() {
    std::vector<Message> readyMessages;
    const int64_t now = nowMs();

    {
        std::lock_guard<std::mutex> lock(mQueueMutex);

        while (!mQueue.empty()) {
            if (mQueue.front().dispatchTimeMs > now) break;
            readyMessages.push_back(std::move(mQueue.front()));
            mQueue.pop_front();
        }
    }

    for (size_t i = 0; i < readyMessages.size(); ++i) {
        dispatchMessage(readyMessages[i]);
    }

    return static_cast<int>(readyMessages.size());
}

void MessageManager::dispatchMessage(const Message& msg) {
    std::vector<MessageListener*> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(mListenerMutex);
        std::unordered_map<int, std::vector<MessageListener*> >::const_iterator it =
            mListeners.find(msg.type);

        if (it == mListeners.end() || it->second.empty())
            return;

        listenersCopy = it->second;
    }

    for (size_t i = 0; i < listenersCopy.size(); ++i) {
        FATAL_IF(listenersCopy[i] == nullptr,
            "[MessageManager] null listener found while dispatching, msgType=%d", msg.type);
        listenersCopy[i]->onMessage(msg.type, msg.value, msg.ptr);
    }
}
