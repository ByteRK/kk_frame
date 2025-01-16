/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2025-01-16 00:10:44
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

#include "common.h"
#include "struct.h"

#define g_connMgr CConnMgr::ins()

class CConnMgr : public EventHandler, public IHandler {

private:
    IPacketBuffer*   mPacket;            // 数据包
    UartClient*      mUartMCU;           // MCU串口
    int64_t          mNextEventTime;     // 下次事件时间
    int64_t          mLastSendTime;      // 最后发送时间
    int64_t          mLastAcceptTime;    // 最后接收时间
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
    int checkEvents();
    int handleEvents();

    // 发送给MCU
    void send2MCU();
    
    // 处理相应
    void onCommDeal(IAck* ack);
public:
};

#endif
