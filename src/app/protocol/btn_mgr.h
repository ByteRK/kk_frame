/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2026-07-06 11:03:14
 * @FilePath: /kk_frame/src/app/protocol/btn_mgr.h
 * @Description: 按键板通讯
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BTN_MGR_H__
#define __BTN_MGR_H__

#include "template/singleton.h"
#include "packet_channel.h"
#include "packet_mgr.h"
#include "tick_mgr.h"

#ifdef PRODUCT_X64
#include "tcp_client.h"
typedef PacketChannel<TcpClient>  BtnCommChannel;
#else
#include "uart_client.h"
typedef PacketChannel<UartClient> BtnCommChannel;
#endif

#define g_btnMgr BtnMgr::instance()

class BtnMgr : public TickMgr::ITickClass, public PacketHandler,
    public Singleton<BtnMgr> {
    friend Singleton<BtnMgr>;
private:
    BtnCommChannel*   mChannel{ nullptr };     // 按键通讯通道
    PacketBufferPool* mPacket{ nullptr };      // 按键数据包缓存池

private:
    bool              mInitialized{ false };   // 初始化完成标志
    int64_t           mLastAcceptTime{ 0 };    // 上次接收时间
    int               mBtnUpd{ 0 };            // 按键更新标志

protected:
    BtnMgr();
    ~BtnMgr();

public:
    int init();

protected:
    void onTick(int64_t nowMs) override;

    void send2Btn();
    void onCommDeal(const IAck* ack) override;

public:
};

#endif // !__BTN_MGR_H__
