/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-02-08 12:34:20
 * @FilePath: /kk_frame/src/app/protocol/conn_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "conn_mgr.h"
#include "mcu_packet.h"
#include "string_utils.h"
#include <core/app.h>
#include <core/systemclock.h>

#define TICK_TIME 100 // tick触发时间（毫秒）

typedef IPacketBufferT<BT_MCU, McuAsk, McuAck> McuPacketBuffer;

ConnMgr::ConnMgr() {
    mPacket = new McuPacketBuffer();
    mNextEventTime = 0;
    mNextSendTime = 0;
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    mMcuUpd = 0;
    mUartMcu = nullptr;
    mInitialized = false;
}

ConnMgr::~ConnMgr() {
    if (mInitialized) {
        cdroid::App::getInstance().removeEventHandler(this);
        g_packetMgr->removeHandler(this);
        mInitialized = false;
    }
    if (mUartMcu) {
        delete mUartMcu;
        mUartMcu = nullptr;
    }
    if (mPacket) {
        delete mPacket;
        mPacket = nullptr;
    }
}

int ConnMgr::init() {
    if (mInitialized) {
        return 0;
    }

    LOGI("ConnMgr init");
    UartClient::Config config;
    config.device = "/dev/ttyS1";
    config.baudRate = 9600;
    config.flowControl = 0;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = 'N';
    config.pollIntervalMs = 10;

    ConnCommChannel* channel = new ConnCommChannel(mPacket, false, config);
    const int rc = channel->init();
    if (rc != 0) {
        LOGE("ConnMgr channel init failed. rc=%d", rc);
        delete channel;
        return rc;
    }
    if (!g_packetMgr->addHandler(BT_MCU, this)) {
        LOGE("ConnMgr packet handler registration failed");
        delete channel;
        return -4;
    }
    mUartMcu = channel;

    // 启动延迟一会后开始发包
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + TICK_TIME * 10;
    cdroid::App::getInstance().addEventHandler(this);
    mInitialized = true;
    return 0;
}

int ConnMgr::checkEvents() {
    int64_t now = cdroid::SystemClock::uptimeMillis();
    if (mUartMcu && mMcuUpd > 0) return 1;
    if (now >= mNextEventTime) {
        mNextEventTime = now + TICK_TIME;
        return 1;
    }
    return 0;
}

int ConnMgr::handleEvents() {
    if (mUartMcu) {
        if (mMcuUpd > 0) send2Mcu();
        mUartMcu->onTick();

        if (cdroid::SystemClock::uptimeMillis() - mLastAcceptTime > 10 * 1000) {
            LOGE("mcu communication failure");
        }
    }
    return 1;
}

/// @brief 发送串口消息
void ConnMgr::send2Mcu() {
    BuffData* bd = mPacket->obtain(false);
    if (bd == nullptr) {
        LOGE("ConnMgr packet allocation failed");
        return;
    }
    McuAsk    snd(bd);

    // TODO:设置数据

    snd.checkCode();
    LOG(VERBOSE) << "[CONN -->] hex str: " << StringUtils::hexStr(bd->buf, bd->len);
    mUartMcu->send(bd);
}

/// @brief 处理串口信息
/// @param ack 
void ConnMgr::onCommDeal(IAck* ack) {

    // TODO:解析处理

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    LOG(VERBOSE) << "[<-- CONN] hex str: " << StringUtils::hexStr(ack->mBuf, ack->mDlen);
}
