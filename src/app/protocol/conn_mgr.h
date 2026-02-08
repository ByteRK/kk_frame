/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2026-02-08 12:10:58
 * @FilePath: /kk_frame/src/app/protocol/conn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CONN_MGR_H__
#define __CONN_MGR_H__

#include "packet_buffer.h"
#include "uart_client.h"
#include "packet_handler.h"
#include "template/singleton.h"

#define g_connMgr ConnMgr::instance()

class ConnMgr : public EventHandler, public IHandler,
    public Singleton<ConnMgr>{
    friend Singleton<ConnMgr>;
private:
    IPacketBuffer*   mPacket;                  // 电控数据包
    int64_t          mNextEventTime;           // 下次事件时间
    int64_t          mNextSendTime;            // 下次发送时间
    int64_t          mLastAcceptTime;          // 上次接收时间
    int              mMcuUpd;                  // 电控更新标志
    UartClient*      mUartMcu;                 // 电控串口

protected:
    ConnMgr();
    ~ConnMgr();

public:
    int init();

protected:
    int checkEvents() override;
    int handleEvents() override;

    void send2Mcu();
    void onCommDeal(IAck* ack) override;
    
public:
};

#endif // !__CONN_MGR_H__
