/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2026-02-08 12:11:46
 * @FilePath: /kk_frame/src/app/protocol/btn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BTN_MGR_H__
#define __BTN_MGR_H__

#include "packet_buffer.h"
#include "uart_client.h"
#include "packet_handler.h"
#include "template/singleton.h"

#define g_btnMgr BtnMgr::instance()

class BtnMgr : public EventHandler, public IHandler,
    public Singleton<BtnMgr>{
    friend Singleton<BtnMgr>;
private:
    IPacketBuffer*   mPacket;                  // 按键数据包
    int64_t          mNextEventTime;           // 下次事件时间
    int64_t          mNextSendTime;            // 下次发送时间
    int64_t          mLastAcceptTime;          // 上次接收时间
    int              mBtnUpd;                  // 按键更新标志
    UartClient*      mUartBtn;                 // 按键串口

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
