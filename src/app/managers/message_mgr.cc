/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-02 17:41:09
 * @LastEditTime: 2026-04-02 19:16:04
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


MessageManager::MessageManager()
    : mMainThreadBound(false) { }

MessageManager::~MessageManager() { }

int64_t MessageManager::nowMs() {
    return cdroid::SystemClock::uptimeMillis();
}

void MessageManager::bindMainThread() {
    std::lock_guard<std::mutex> lock(mStateMutex);
    if (!mMainThreadBound) {
        mMainThreadId = std::this_thread::get_id();
        mMainThreadBound = true;
        return;
    }
    MSG_MGR_FATAL_IF(mMainThreadId != std::this_thread::get_id(),
        "bindMainThread() called again from different thread");
}

void MessageManager::add(int msgType, MessageListener* listener) {
    MSG_MGR_CHECK_MAIN_THREAD();
    MSG_MGR_FATAL_IF(listener == nullptr, "add() got null listener, msgType=%d", msgType);

    std::lock_guard<std::mutex> lock(mListenerMutex);

    std::vector<MessageListener*>& listeners = mListeners[msgType];
    if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end()) {
        MSG_MGR_FATAL("duplicate listener registration, msgType=%d", msgType);
    }

    listeners.push_back(listener);
}

void MessageManager::remove(int msgType, MessageListener* listener) {
    MSG_MGR_CHECK_MAIN_THREAD();
    MSG_MGR_FATAL_IF(listener == nullptr, "remove() got null listener, msgType=%d", msgType);

    std::lock_guard<std::mutex> lock(mListenerMutex);

    std::unordered_map<int, std::vector<MessageListener*> >::iterator it = mListeners.find(msgType);
    MSG_MGR_FATAL_IF(it == mListeners.end(),
        "remove() failed, msgType=%d not found", msgType);

    std::vector<MessageListener*>& listeners = it->second;
    std::vector<MessageListener*>::iterator lit =
        std::find(listeners.begin(), listeners.end(), listener);

    MSG_MGR_FATAL_IF(lit == listeners.end(),
        "remove() failed, listener not found, msgType=%d", msgType);

    listeners.erase(lit);
    if (listeners.empty()) {
        mListeners.erase(it);
    }
}

void MessageManager::removeAll(MessageListener* listener) {
    MSG_MGR_CHECK_MAIN_THREAD();
    MSG_MGR_FATAL_IF(listener == nullptr, "removeAll() got null listener");

    std::lock_guard<std::mutex> lock(mListenerMutex);

    for (std::unordered_map<int, std::vector<MessageListener*> >::iterator it = mListeners.begin();
        it != mListeners.end();) {
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
    MSG_MGR_CHECK_MAIN_THREAD();

    Message msg;
    msg.type = msgType;
    msg.value = msgValue;
    msg.ptr = msgPtr;
    msg.dispatchTimeMs = nowMs();

    dispatchMessage(msg);
}

void MessageManager::post(int msgType, int msgValue, void* msgPtr, int64_t delayMs) {
    MSG_MGR_CHECK_WORKER_THREAD();
    MSG_MGR_FATAL_IF(delayMs < 0, "post() got negative delay, msgType=%d, delay=%lld",
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

    std::deque<Message>::iterator pos = mQueue.end();
    for (std::deque<Message>::iterator it = mQueue.begin(); it != mQueue.end(); ++it) {
        if (msg.dispatchTimeMs < it->dispatchTimeMs) {
            pos = it;
            break;
        }
    }

    mQueue.insert(pos, std::move(msg));
}

size_t MessageManager::clear(int msgType) {
    std::lock_guard<std::mutex> lock(mQueueMutex);

    size_t oldSize = mQueue.size();

    for (std::deque<Message>::iterator it = mQueue.begin(); it != mQueue.end();) {
        if (it->type == msgType) {
            it = mQueue.erase(it);
        } else {
            ++it;
        }
    }

    return oldSize - mQueue.size();
}

int MessageManager::checkEvents() {
    MSG_MGR_CHECK_MAIN_THREAD();

    const int64_t now = nowMs();
    int readyCount = 0;

    std::lock_guard<std::mutex> lock(mQueueMutex);

    for (std::deque<Message>::const_iterator it = mQueue.begin(); it != mQueue.end(); ++it) {
        if (it->dispatchTimeMs <= now) {
            ++readyCount;
        } else {
            break;
        }
    }

    return readyCount;
}

int MessageManager::handleEvents() {
    MSG_MGR_CHECK_MAIN_THREAD();

    std::vector<Message> readyMessages;
    const int64_t now = nowMs();

    {
        std::lock_guard<std::mutex> lock(mQueueMutex);

        while (!mQueue.empty()) {
            if (mQueue.front().dispatchTimeMs > now) {
                break;
            }

            readyMessages.push_back(std::move(mQueue.front()));
            mQueue.pop_front();
        }
    }

    for (size_t i = 0; i < readyMessages.size(); ++i) {
        dispatchMessage(readyMessages[i]);
    }

    return static_cast<int>(readyMessages.size());
}

void MessageManager::ensureMainThreadBound() const {
    std::lock_guard<std::mutex> lock(mStateMutex);
    MSG_MGR_FATAL_IF(!mMainThreadBound,
        "main thread not bound, call bindMainThread() first");
}

void MessageManager::ensureMainThread() const {
    std::lock_guard<std::mutex> lock(mStateMutex);
    MSG_MGR_FATAL_IF(!mMainThreadBound,
        "main thread not bound, call bindMainThread() first");
    MSG_MGR_FATAL_IF(mMainThreadId != std::this_thread::get_id(),
        "this API must be called from main thread");
}

void MessageManager::ensureWorkerThread() const {
    std::lock_guard<std::mutex> lock(mStateMutex);
    MSG_MGR_FATAL_IF(!mMainThreadBound,
        "main thread not bound, call bindMainThread() first");
    MSG_MGR_FATAL_IF(mMainThreadId == std::this_thread::get_id(),
        "this API must be called from worker thread");
}

void MessageManager::dispatchMessage(const Message& msg) {
    std::vector<MessageListener*> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(mListenerMutex);

        std::unordered_map<int, std::vector<MessageListener*> >::const_iterator it =
            mListeners.find(msg.type);

        // 允许没有监听者，直接忽略
        if (it == mListeners.end() || it->second.empty()) {
            return;
        }

        listenersCopy = it->second;
    }

    for (size_t i = 0; i < listenersCopy.size(); ++i) {
        MSG_MGR_FATAL_IF(listenersCopy[i] == nullptr,
            "null listener found while dispatching, msgType=%d", msg.type);

        listenersCopy[i]->onMessage(msg.type, msg.value, msg.ptr);
    }
}