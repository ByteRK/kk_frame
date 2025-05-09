/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:55:30
 * @FilePath: /hana_frame/src/protocol/conn_mgr.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
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
