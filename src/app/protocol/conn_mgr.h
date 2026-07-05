/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:54:27
 * @LastEditTime: 2026-07-05 23:17:33
 * @FilePath: /kk_frame/src/app/protocol/conn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CONN_MGR_H__
#define __CONN_MGR_H__

#include "template/singleton.h"
#include "packet_channel.h"
#include "packet_mgr.h"
#include "tick_mgr.h"

#ifdef PRODUCT_X64
#include "tcp_client.h"
typedef PacketChannel<TcpClient>  ConnCommChannel;
#else
#include "uart_client.h"
typedef PacketChannel<UartClient> ConnCommChannel;
#endif

#define g_connMgr ConnMgr::instance()

class ConnMgr : public TickMgr::ITickClass, public PacketHandler,
    public Singleton<ConnMgr> {
    friend Singleton<ConnMgr>;
private:
    ConnCommChannel*  mChannel{ nullptr };      // 电控通讯通道
    PacketBufferPool* mPacket{ nullptr };       // 电控数据包缓存池

private:
    bool              mInitialized{ false };    // 初始化完成标志
    int64_t           mLastAcceptTime{ 0 };     // 上次接收时间
    int               mMcuUpd{ 0 };             // 电控更新标志

protected:
    ConnMgr();
    ~ConnMgr();

public:
    int init();

protected:
    void onTick(int64_t nowMs) override;

    void send2Mcu();
    void onCommDeal(const IAck* ack) override;

public:
};

#endif // !__CONN_MGR_H__
