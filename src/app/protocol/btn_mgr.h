/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2026-02-08 12:33:47
 * @FilePath: /kk_frame/src/app/protocol/btn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BTN_MGR_H__
#define __BTN_MGR_H__

#include "comm/packet/packet_buffer.h"
#include "comm/packet/packet_channel.h"
#include "uart_client.h"
#include "comm/packet/packet_handler.h"
#include "template/singleton.h"

#include <core/looper.h>

#define g_btnMgr BtnMgr::instance()

typedef PacketChannel<UartClient> BtnCommChannel;

class BtnMgr : public cdroid::EventHandler, public IHandler,
    public Singleton<BtnMgr> {
    friend Singleton<BtnMgr>;
private:
    IPacketBuffer*   mPacket;                  // 按键数据包
    int64_t          mNextEventTime;           // 下次事件时间
    int64_t          mNextSendTime;            // 下次发送时间
    int64_t          mLastAcceptTime;          // 上次接收时间
    int              mBtnUpd;                  // 按键更新标志
    BtnCommChannel*  mUartBtn;                 // 按键通讯通道
    bool             mInitialized;             // 初始化完成标志

protected:
    BtnMgr();
    ~BtnMgr();

public:
    int init();

protected:
    int checkEvents() override;
    int handleEvents() override;

    void send2Btn();
    void onCommDeal(IAck* ack) override;

public:
};

#endif // !__BTN_MGR_H__
