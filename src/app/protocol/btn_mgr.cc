/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-02-08 12:34:13
 * @FilePath: /kk_frame/src/app/protocol/btn_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "btn_mgr.h"
#include "btn_packet.h"
#include "string_utils.h"
#include <core/app.h>
#include <core/systemclock.h>

#define TICK_TIME 100 // tick触发时间（毫秒）

typedef IPacketBufferT<BT_BTN, BtnAsk, BtnAck> BtnPacketBuffer;

BtnMgr::BtnMgr() {
    mPacket = new BtnPacketBuffer();
    mNextEventTime = 0;
    mNextSendTime = 0;
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    mBtnUpd = 0;
    mUartBtn = nullptr;

    g_packetMgr->addHandler(BT_BTN, this);
}

BtnMgr::~BtnMgr() {
    if (mUartBtn) {
        delete mUartBtn;
        mUartBtn = nullptr;
    }
}

int BtnMgr::init() {
    LOGI("BtnMgr init");
    UartClient::Config config;
    config.device = "/dev/ttyS2";
    config.baudRate = 9600;
    config.flowControl = 0;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = 'N';
    config.pollIntervalMs = 10;

    mUartBtn = new BtnCommChannel(mPacket, false, config);
    mUartBtn->init();

    // 启动延迟一会后开始发包
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + TICK_TIME * 10;
    cdroid::App::getInstance().addEventHandler(this);
    return 0;
}

int BtnMgr::checkEvents() {
    int64_t now = cdroid::SystemClock::uptimeMillis();
    if (mUartBtn && mBtnUpd > 0) return 1;
    if (now >= mNextEventTime) {
        mNextEventTime = now + TICK_TIME;
        return 1;
    }
    return 0;
}

int BtnMgr::handleEvents() {
    if (mUartBtn) {
        if (mBtnUpd > 0) send2Btn();
        mUartBtn->onTick();

        if (cdroid::SystemClock::uptimeMillis() - mLastAcceptTime > 10 * 1000) {
            LOGE("btn communication failure");
        }
    }
    return 1;
}

/// @brief 发送串口消息
void BtnMgr::send2Btn() {
    BuffData* bd = mPacket->obtain(false);
    BtnAsk    snd(bd);

    // TODO:设置数据

    snd.checkCode();
    LOG(VERBOSE) << "[BTN -->] hex str: " << StringUtils::hexStr(bd->buf, bd->len);
    mUartBtn->send(bd);
}

/// @brief 处理串口信息
/// @param ack 
void BtnMgr::onCommDeal(IAck* ack) {

    // TODO:解析处理

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    LOG(VERBOSE) << "[<-- BTN] hex str: " << StringUtils::hexStr(ack->mBuf, ack->mDlen);
}
