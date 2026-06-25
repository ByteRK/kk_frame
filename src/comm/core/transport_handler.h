/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/core/transport_handler.h
 * @Description: Raw transport callback interface
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_HANDLER_H__
#define __TRANSPORT_HANDLER_H__

#include <stddef.h>
#include <stdint.h>

class TransportHandler {
public:
    virtual ~TransportHandler() { }

    virtual void onConnected(int id = -1) { }
    virtual void onDisconnected(int id = -1) { }
    virtual void onRecv(const uint8_t* data, size_t len, int id = -1) = 0;
    virtual void onError(int err) { }
};

#endif // !__TRANSPORT_HANDLER_H__
