/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2024-08-23 10:16:01
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
#include "global_data.h"

#include "common.h"
#include "struct.h"

enum { // 按键灯亮度
    BTN_HIGHT = 0x99,
    BTN_LOW   = 0x27,
    BTN_CLOSE = 0x00,
};

#define g_btnMgr BtnMgr::ins()

class BtnMgr : public EventHandler, public IHandler {

private:
    int64_t          mLastStatusTime;
    IPacketBuffer* mPacket;
    int64_t          mNextEventTime;
    int64_t          mLastSendTime;
    int64_t          mLastWarnTime;
    UartClient* mUartBTN;

    uchar            mVersionL;
    uchar            mVersionR;

    bool             mFirstSend;

    bool             mPower;          // 功能开关
    int64_t          mDownTime;       // 按键按下时间
protected:
    BtnMgr();
    ~BtnMgr();

public:
    static BtnMgr* ins() {
        static BtnMgr stIns;
        return &stIns;
    }

    int init();

    std::string getVersion(); // 获取版本号

protected:
    virtual int checkEvents();
    virtual int handleEvents();

    // 发送给MCU
    void send2MCU();

    // 处理相应
    virtual void onCommDeal(IAck* ack);

    /// @brief 处理转发按键
    /// @param key 
    /// @param status 
    void sendButton(ushort key, uchar status);

    ///////////////////////////// 控制命令 /////////////////////////////
public:

};

#endif
