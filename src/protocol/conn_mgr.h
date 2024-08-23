/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2024-08-23 13:57:25
 * @FilePath: /kk_frame/src/protocol/conn_mgr.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __conn_mgr_h__
#define __conn_mgr_h__

#include "packet_buffer.h"
#include "uart_client.h"
#include "cmd_handler.h"
#include "global_data.h"

#include "common.h"
#include "struct.h"

#define g_connMgr CConnMgr::ins()

class CConnMgr : public EventHandler, public IHandler {

private:
    int64_t          mLastStatusTime;
    IPacketBuffer*   mPacket;
    int64_t          mNextEventTime;
    int64_t          mLastSendTime;
    int64_t          mLastAcceptTime;
    int64_t          mLastWarnTime;
    UartClient*      mUartMCU; // mcu
protected:
    CConnMgr();
    ~CConnMgr();

public:
    static CConnMgr* ins() {
        static CConnMgr stIns;
        return &stIns;
    }

    int init();

protected:
    virtual int checkEvents();
    virtual int handleEvents();

    // 发送给MCU
    void send2MCU();
    // 处理相应
    virtual void onCommDeal(IAck* ack);

public:
};

#endif
