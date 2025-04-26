/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2025-02-18 19:19:58
 * @FilePath: /cy_frame/src/protocol/conn_mgr.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */

#ifndef __conn_mgr_h__
#define __conn_mgr_h__

#include "packet_buffer.h"
#include "uart_client.h"
#include "packet_handler.h"

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
    std::string getVersion();
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
