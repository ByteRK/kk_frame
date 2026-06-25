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

#include <queue>
#include <mutex>
#include <stdint.h>
#include <sys/types.h>
#include <vector>

class Transport : public cdroid::EventHandler {
public:
    struct Event {
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

    virtual void setHandler(TransportHandler* handler) = 0;
    virtual int init() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const = 0;
    virtual void onTick() { }
    virtual ssize_t send(const uint8_t* data, size_t len, int id = -1) = 0;

    int checkEvents() override;
    int handleEvents() override;

protected:
    int initEventDispatcher(cdroid::Looper* mainLooper = nullptr);
    void shutdownEventDispatcher();
    bool isEventDispatcherReady() const;

    void postEvent(const Event& ev);
    void sendEvent(const Event& ev);
    virtual void dispatchEvent(const Event& ev) = 0;

private:
    static int onWake(int fd, int events, void* context);
    void wakeMainThread();
    void drainWakeFd();

private:
    mutable std::mutex mEventLock;
    std::queue<Event> mEvents;
    cdroid::Looper* mMainLooper;
    int mWakeFd;
};

#endif // !__TRANSPORT_H__
