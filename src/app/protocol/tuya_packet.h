/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/app/protocol/tuya_packet.h
 * @Description: Tuya protocol packets
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TUYA_PACKET_H__
#define __TUYA_PACKET_H__

#include "packet_base.h"
#include "proto.h"
#include "check_utils.h"
#include "string_utils.h"

#include <cdlog.h>

class TuyaAsk : public IAsk {
public:
    /// @brief 不含可变载荷的发送包基础长度。
    constexpr static uint16_t BASE_LEN = 7;

public:
    TuyaAsk() { }
    TuyaAsk(BuffData* buf) { parse(buf); }

    void parse(BuffData* buf) override {
        IAsk::parse(buf);
        mBuf->buf[0] = 0x55;
        mBuf->buf[1] = 0xAA;
    }

    void checkCode() override {
        mBuf->buf[mBuf->len - 1] = CheckUtils::checkSum(mBuf->buf, mBuf->len - 1) % 0x100;
    }
};

class TuyaAck : public IAck {
public:
    /// @brief 接收缓存容量上限，即允许解析的最大完整包长度。
    constexpr static uint16_t BUFFER_CAPACITY = 0x466;
    /// @brief 除可变载荷外的固定帧开销，包含包头、控制字段和校验字节。
    constexpr static uint16_t FRAME_OVERHEAD = 7;
    /// @brief 能够读取载荷长度字段时所需的最小缓存长度。
    constexpr static uint16_t LENGTH_READY_LEN = 6;

private:
    uint8_t mHeadList[2] = { 0x55, 0xAA };

public:
    TuyaAck() { }
    TuyaAck(BuffData* buf) { parse(buf); }

    bool check() override {
        if (!complete()) return false;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    }

    int getType() override {
        return BT_TUYA;
    }

protected:
    const uint8_t* head() const override { return mHeadList; }
    uint16_t headLength() const override { return sizeof(mHeadList); }
    uint16_t lengthReadySize() const override { return LENGTH_READY_LEN; }

    int32_t expectedLength() const override {
        const uint16_t payloadLen =
            (static_cast<uint16_t>(mBuf[4]) << 8) | mBuf[5];
        return static_cast<int32_t>(payloadLen) + FRAME_OVERHEAD;
    }
};

#endif // !__TUYA_PACKET_H__
