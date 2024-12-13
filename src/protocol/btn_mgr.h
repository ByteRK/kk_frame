/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-06-12 14:49:06
 * @LastEditTime: 2024-11-06 10:35:12
 * @FilePath: /kaidu_t2e_pro/src/protocol/btn_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef __btn_mgr_h__
#define __btn_mgr_h__

#include "packet_buffer.h"
#include "i2c_client.h"
#include "cmd_handler.h"
#include "global_data.h"

#include "common.h"
#include "struct.h"

#define LEFT_BTN_COUNT 9
#define RIGHT_BTN_COUNT 9
#define ALL_BTN_COUNT 18

enum { // 按键灯亮度
    // BTN_HIGHT = 0x5A,
    // BTN_LOW =   0x12,
    // BTN_CLOSE = 0x00,
    BTN_HIGHT = 0xB2,
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
    I2CClient* mI2CBTN;

    uchar            mVersionL;
    uchar            mVersionR;

    bool             mFirstSend;       // 第一次发送
    bool             mSendLeft;        // 上一次是否发送左边按键
    uchar            mBtnLight[ALL_BTN_COUNT];    // 按键灯列表

    int64_t          mDownTime;       // 按键按下时间
    uchar            LK_TEST;    // 长按下键+启停进入测试模式计数
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
    /// @param type 
    /// @param key 
    /// @param status 
    void sendButton(uchar type, ushort key, uchar status);

    /// @brief 处理多按键长按
    /// @param type 
    /// @param value 
    void dealLK(uchar type, ushort value);

    ///////////////////////////// 控制命令 /////////////////////////////
public:
    void setLight(uchar* left, uchar* right);
};

#endif
