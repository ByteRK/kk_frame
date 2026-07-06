/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-07-06 11:03:19
 * @FilePath: /kk_frame/src/app/protocol/btn_mgr.cc
 * @Description:按键板通讯
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "btn_mgr.h"
#include "btn_packet.h"
#include "string_utils.h"
#include "project_utils.h"
#include "app_common.h"
#include <core/app.h>
#include <core/systemclock.h>

#define TICK_TIME 100 // tick触发时间（毫秒）

typedef PacketBufferPoolT<BT_BTN, BtnAsk, BtnAck> BtnPacketBufferPool;

BtnMgr::BtnMgr() {
    mPacket = new BtnPacketBufferPool();
    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
}

BtnMgr::~BtnMgr() {
    mInitialized = false;
    stopTick();
    __delete(mChannel);
    __delete(mPacket);
}

int BtnMgr::init() {
    if (mInitialized) return 0;
    LOGI("BtnMgr init");

#ifdef PRODUCT_X64
    TcpClient::Config config;
    ProjectUtils::getDebugServiceInfo(config.host, config.port);
    config.port += BT_BTN;
#else
    UartClient::Config config;
    config.device = "/dev/ttyS2";
    config.baudRate = 9600;
    config.flowControl = 0;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = 'N';
    config.pollIntervalMs = 200;
#endif

    BtnCommChannel* channel = new BtnCommChannel(mPacket, false, config);
    const int rc = channel->init();
    if (rc != 0) {
        LOGE("BtnMgr channel init failed. rc=%d", rc);
        delete channel;
        return rc;
    }
    if (!g_packetMgr->addHandler(BT_BTN, this)) {
        LOGE("BtnMgr packet handler registration failed");
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

void BtnMgr::onTick(int64_t nowMs) {
    if (mChannel) {
        if (mBtnUpd > 0) send2Btn();

        if (nowMs - mLastAcceptTime > 10 * 1000) {
            LOGE("btn communication failure");
            mLastAcceptTime = nowMs;
        }
    }
}

/// @brief 发送串口消息
void BtnMgr::send2Btn() {
    BuffData* bd = mPacket->obtainSend();
    if (bd == nullptr) {
        LOGE("BtnMgr packet allocation failed");
        return;
    }
    BtnAsk    snd(bd);

    // TODO:设置数据

    snd.checkCode();
    LOG(VERBOSE) << "[BTN -->] hex str: " << StringUtils::hexStr(bd->buf, bd->len);
    mChannel->send(bd);
}

/// @brief 处理串口信息
/// @param ack 
void BtnMgr::onCommDeal(const IAck* ack) {

    // TODO:解析处理

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
    LOG(VERBOSE) << "[<-- BTN] hex str: " << StringUtils::hexStr(ack->data(), ack->dataLength());
}
