/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/app/protocol/mcu_packet.h
 * @Description: MCU protocol packets
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __MCU_PACKET_H__
#define __MCU_PACKET_H__

#include "packet_base.h"
#include "proto.h"
#include "check_utils.h"
#include "string_utils.h"

#include <cdlog.h>

class McuAsk : public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 23;

public:
    McuAsk() { }
    McuAsk(BuffData* buf) { parse(buf); }

    void parse(BuffData* buf) override {
        IAsk::parse(buf);
        mBuf->buf[0] = 0xA5;
        mBuf->buf[1] = 0x5A;
    }

    void checkCode() override {
        mBuf->buf[mBuf->len - 1] = CheckUtils::checkSum(mBuf->buf + 2, mBuf->len - 3) & 0xFF;
    }
};

class McuAck : public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0xFF;
    constexpr static uint8_t MIN_LEN = 36;

private:
    uint8_t mHeadList[2] = { 0xA5, 0x5A };

public:
    McuAck() { }
    McuAck(BuffData* buf) { parse(buf); }

    bool check() override {
        if (!complete()) return false;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    }

    int getType() override {
        return BT_MCU;
    }

protected:
    const uint8_t* head() const override { return mHeadList; }
    uint16_t headLength() const override { return sizeof(mHeadList); }
    uint16_t lengthReadySize() const override { return headLength(); }
    int32_t expectedLength() const override { return MIN_LEN; }
};

#endif // !__MCU_PACKET_H__
