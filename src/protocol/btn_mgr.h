/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2025-01-16 00:05:44
 * @FilePath: /kk_frame/src/protocol/btn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef __btn_mgr_h__
#define __btn_mgr_h__

#include "packet_buffer.h"
#include "uart_client.h"
#include "cmd_handler.h"

// 按键数量
constexpr int LEFT_BTN_COUNT = 6;
constexpr int RIGHT_BTN_COUNT = 5;
constexpr int ALL_BTN_COUNT = LEFT_BTN_COUNT + RIGHT_BTN_COUNT;

// 按键灯亮度
constexpr uint8_t BTN_HIGHT   = 0xFF * 100 / 100 ; // 高亮
constexpr uint8_t BTN_LOW     = 0xFF * 30  / 100 ; // 低亮
constexpr uint8_t BTN_CLOSE   = 0xFF * 0   / 100 ; // 关闭

#define g_btnMgr BtnMgr::ins()

class BtnMgr : public EventHandler, public IHandler {
private:
    IPacketBuffer*   mPacket;
    int64_t          mNextEventTime;
    int64_t          mLastSendTime;
    UartClient*      mUartMCU; // 按键串口

    uint8_t            mVersionL;
    uint8_t            mVersionR;
    uint8_t            mBtnLight[ALL_BTN_COUNT]; // 按键灯状态列表
    bool             mBtnLightChanged;         // 按键灯状态是否改变
protected:
    BtnMgr();
    ~BtnMgr();
public:
    static BtnMgr* ins() {
        static BtnMgr stIns;
        return &stIns;
    }
    int init();
    std::string getVersion();
protected:
    int checkEvents() override;
    int handleEvents() override;

    void send2MCU();
    void onCommDeal(IAck* ack) override;
public:
    void setLight(uint8_t* light);
};

#endif
