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
#include "string_utils.h"
#include "project_utils.h"
#include <core/app.h>

#define TICK_TIME 100 // tick触发时间（毫秒）

typedef IPacketBufferT<BT_MCU, McuAsk, McuAck> McuPacketBuffer;

ConnMgr::ConnMgr() {
    mPacket = new McuPacketBuffer();
    mNextEventTime = 0;
    mNextSendTime = 0;
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    mMcuUpd = 0;
    mUartMcu = nullptr;

    g_packetMgr->addHandler(BT_MCU, this);
}

ConnMgr::~ConnMgr() {
    if (mUartMcu) {
        delete mUartMcu;
        mUartMcu = nullptr;
    }
}

int ConnMgr::init() {
    LOGI("ConnMgr init");
    UartOpenReq ss;

    snprintf(ss.serialPort, sizeof(ss.serialPort), "/dev/ttyS1");
    ss.speed = 9600;
    ss.flow_ctrl = 0;
    ss.databits = 8;
    ss.stopbits = 1;
    ss.parity = 'N';

    std::string debugIp;
    short debugPort;
    ProjectUtils::getDebugServiceInfo(debugIp, debugPort);
    mUartMcu = new UartClient(mPacket, ss, debugIp, debugPort + BT_MCU, 0);
    mUartMcu->init();

    // 启动延迟一会后开始发包
    mNextEventTime = SystemClock::uptimeMillis() + TICK_TIME * 10;
    App::getInstance().addEventHandler(this);
    return 0;
}

int ConnMgr::checkEvents() {
    int64_t now = SystemClock::uptimeMillis();
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
    BtnAsk    snd(bd);

    // TODO:设置数据

    snd.checkCode();
    LOG(VERBOSE) << "[CONN -->] hex str: " << StringUtils::hexStr(bd->buf, bd->len);
    mUartMcu->send(bd);
}

/// @brief 处理串口信息
/// @param ack 
void ConnMgr::onCommDeal(IAck* ack) {

    // TODO:解析处理

    mLastAcceptTime = SystemClock::uptimeMillis();
    LOG(VERBOSE) << "[<-- CONN] hex str: " << StringUtils::hexStr(ack->mBuf, ack->mDlen);
}
