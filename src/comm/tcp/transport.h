/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:20:18
 * @LastEditTime: 2026-02-05 18:01:48
 * @FilePath: /kk_frame/src/comm/tcp/transport.h
 * @Description: Transport 抽象基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include <queue>
#include <mutex>
#include <vector>
#include <cstdint>

#include <core/looper.h>

/// @brief 事件定义
struct TransportEvent {
    enum Type {
        CONNECTED,
        DISCONNECTED,
        DATA,
        ERROR,
        CLIENT_CONNECTED,
        CLIENT_DISCONNECTED,
    } type;

    int clientId{ -1 };   // server 用
    int error{ 0 };
    std::vector<uint8_t> data;
};

/// @brief Transport 抽象基类
class Transport : public cdroid::EventHandler {
public:
    virtual ~Transport();

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual ssize_t send(const uint8_t* data, size_t len, int clientId = -1) = 0;

    int checkEvents() override;
    int handleEvents() override;

protected:
    Transport();

    void postEvent(const TransportEvent& ev);

    virtual void dispatchEvent(const TransportEvent& ev) = 0;

private:
    std::queue<TransportEvent> mEvents;
    std::mutex mEventLock;
};

#endif // !__TRANSPORT_H__