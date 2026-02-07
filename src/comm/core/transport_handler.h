/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-07 16:47:18
 * @LastEditTime: 2026-02-07 16:58:48
 * @FilePath: /kk_frame/src/comm/core/transport_handler.h
 * @Description: 数据传输相关事件消费
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_HANDLER_H__
#define __TRANSPORT_HANDLER_H__

#include <stdint.h>
#include <stddef.h>

class TransportHandler {
public:
    virtual ~TransportHandler() { }

    virtual void onConnected(int id = -1) { }               // 连接成功
    virtual void onDisconnected(int id = -1) { }            // 连接断开
    virtual void onRecv(                                    // 接收数据
        const uint8_t* data, size_t len, int id = -1) = 0;
    virtual void onError(int err) { }                       // 错误
};

#endif // !__TRANSPORT_HANDLER_H__
