/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2026-02-06 11:33:55
 * @FilePath: /kk_frame/src/app/protocol/conn_mgr.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2025 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __CONN_MGR_H__
#define __CONN_MGR_H__

#include "packet_buffer.h"
#include "uart_client.h"
#include "packet_handler.h"
#include "template/singleton.h"

#include "struct.h"

#define g_connMgr CConnMgr::instance()

class CConnMgr : public EventHandler, public IHandler,
    public Singleton<CConnMgr>{
    friend Singleton<CConnMgr>;
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

#endif // !__CONN_MGR_H__
