/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:20:18
 * @LastEditTime: 2026-02-07 16:55:25
 * @FilePath: /kk_frame/src/comm/core/transport.h
 * @Description: 数据传输基类
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

/// @brief 数据传输基类
class Transport : public cdroid::EventHandler {
public:
    /// @brief 数据传输事件
    struct Event {
        enum Type {
            CONNECTED,
            DISCONNECTED,
            DATA,
            ERROR,
            CLIENT_CONNECTED,
            CLIENT_DISCONNECTED,
        } type;

        int id{ -1 };   // server用，区分客户端
        int error{ 0 };
        std::vector<uint8_t> data;
    };

private:
    std::mutex mEventLock;                // 事件锁
    std::queue<Transport::Event> mEvents; // 事件队列

protected:
    Transport();

public:
    virtual ~Transport();

    virtual bool start() = 0;             // 启动连接
    virtual void stop() = 0;              // 关闭连接

    virtual ssize_t send(                 // 数据发送(Id为多目标连接时使用)
        const uint8_t* data, size_t len, int id = -1) = 0;

    int checkEvents() override;           // 非UI转UI
    int handleEvents() override;          // 非UI转UI

    void postEvent(const Transport::Event& ev);   // 非UI线程投递
    void sendEvent(const Transport::Event& ev);   // UI线程投递
    virtual void dispatchEvent(
        const Transport::Event& ev) = 0;          // UI线程处理
};

#endif // !__TRANSPORT_H__