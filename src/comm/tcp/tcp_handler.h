/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-07 11:50:23
 * @LastEditTime: 2026-02-07 11:53:23
 * @FilePath: /kk_frame/src/comm/tcp/tcp_handler.h
 * @Description: TCP 事件消费
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __TCP_HANDLER_H__
#define __TCP_HANDLER_H__

#include <stdint.h>
#include <stddef.h>

class TcpHandler {
public:
    virtual ~TcpHandler() { }

    virtual void onConnected(int id = 0) { }
    virtual void onDisconnected(int id = 0) { }
    virtual void onRecv(const uint8_t* data, size_t len, int id = 0) = 0;
    virtual void onError(int err) { }
};

#endif // !__TCP_HANDLER_H__
