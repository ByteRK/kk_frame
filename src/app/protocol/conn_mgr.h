/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2026-07-05 22:18:08
 * @FilePath: /kk_frame/src/app/protocol/conn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CONN_MGR_H__
#define __CONN_MGR_H__

#include "packet_channel.h"
#include "uart_client.h"
#include "packet_mgr.h"
#include "template/singleton.h"

#include <core/looper.h>

#define g_connMgr ConnMgr::instance()

typedef PacketChannel<UartClient> ConnCommChannel;

class ConnMgr : public cdroid::EventHandler, public PacketHandler,
    public Singleton<ConnMgr>{
    friend Singleton<ConnMgr>;
private:
    PacketBufferPool* mPacket;                  // 电控数据包缓存池
    int64_t           mNextEventTime;           // 下次事件时间
    int64_t           mNextSendTime;            // 下次发送时间
    int64_t           mLastAcceptTime;          // 上次接收时间
    int               mMcuUpd;                  // 电控更新标志
    ConnCommChannel*  mUartMcu;                 // 电控通讯通道
    bool              mInitialized;             // 初始化完成标志

protected:
    ConnMgr();
    ~ConnMgr();

public:
    int init();

protected:
    int checkEvents() override;
    int handleEvents() override;

    void send2Mcu();
    void onCommDeal(const IAck* ack) override;
    
public:
};

#endif // !__CONN_MGR_H__
