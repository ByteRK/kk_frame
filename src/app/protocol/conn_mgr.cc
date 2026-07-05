/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-07-06 01:24:28
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
#include "project_utils.h"
#include "app_common.h"
#include <core/app.h>
#include <core/systemclock.h>

#define TICK_TIME 100 // tick触发时间（毫秒）

typedef PacketBufferPoolT<BT_MCU, McuAsk, McuAck> McuPacketBufferPool;

ConnMgr::ConnMgr() {
    mPacket = new McuPacketBufferPool();
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
}

ConnMgr::~ConnMgr() {
    mInitialized = false;
    stopTick();
    __delete(mChannel);
    __delete(mPacket);
}

int ConnMgr::init() {
    if (mInitialized) return 0;
    LOGI("ConnMgr init");

#ifdef PRODUCT_X64
    TcpClient::Config config;
    ProjectUtils::getDebugServiceInfo(config.host, config.port);
    config.port += BT_MCU;
#else
    UartClient::Config config;
    config.device = "/dev/ttyS1";
    config.baudRate = 9600;
    config.flowControl = 0;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = 'N';
    config.pollIntervalMs = 200;
#endif

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
    mChannel = channel;

    // 启动延迟一会后开始发包
    setTick(TICK_TIME);
    startTick(TICK_TIME * 10);

    mInitialized = true;
    return 0;
}

void ConnMgr::onTick(int64_t nowMs) {
    if (mChannel) {
        if (mMcuUpd > 0) send2Mcu();

        if (nowMs - mLastAcceptTime > 10 * 1000) {
            LOGE("mcu communication failure");
            mLastAcceptTime = nowMs;
        }
    }
}

/// @brief 发送串口消息
void ConnMgr::send2Mcu() {
    BuffData* bd = mPacket->obtainSend();
    if (bd == nullptr) {
        LOGE("ConnMgr packet allocation failed");
        return;
    }
    McuAsk    snd(bd);

    // TODO:设置数据

    snd.checkCode();
    LOG(VERBOSE) << "[CONN -->] hex str: " << StringUtils::hexStr(bd->buf, bd->len);
    mChannel->send(bd);
}

/// @brief 处理串口信息
/// @param ack 
void ConnMgr::onCommDeal(const IAck* ack) {

    // TODO:解析处理

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    LOG(VERBOSE) << "[<-- CONN] hex str: " << StringUtils::hexStr(ack->data(), ack->dataLength());
}
