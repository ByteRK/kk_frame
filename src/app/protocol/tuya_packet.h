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

#include "comm/packet/packet_base.h"
#include "proto.h"
#include "check_utils.h"
#include "string_utils.h"

#include <cdlog.h>

class TuyaAsk : public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 7;

public:
    TuyaAsk() { }
    TuyaAsk(BuffData* buf) { parse(buf); }

    void parse(BuffData* buf) override {
        IAsk::parse(buf);
        mBf->buf[0] = 0x55;
        mBf->buf[1] = 0xAA;
    }

    void checkCode() override {
        mBf->buf[mBf->len - 1] = CheckUtils::checkSum(mBf->buf, mBf->len - 1) % 0x100;
    }
};

class TuyaAck : public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0x466;
    constexpr static uint8_t MIN_LEN = 7;

private:
    uint8_t mHeadList[2] = { 0x55, 0xAA };

public:
    TuyaAck() { }
    TuyaAck(BuffData* buf) { parse(buf); }

    int add(const uint8_t* bf, int len) override {
        int rlen = 0;
        while (mDlen < BUF_LEN && rlen < len) {
            addData(BUF_LEN, bf, len, rlen);
            if (!checkHead(mHeadList, 2))findHead(mHeadList, 2);
            *mPlen = mDlen;
        }
        // 裁剪真实数据长度，避免数据包拼接导致遗漏
        if (checkHead(mHeadList, 2) && mDlen > MIN_LEN) {
            uint16_t realLen = (mBuf[4] << 8) | mBuf[5] + MIN_LEN;
            if (mDlen > realLen) {
                rlen -= (mDlen - realLen);
                rlen = rlen < 0 ? 0 : rlen;
            }
        }
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << StringUtils::hexStr(mBuf, mDlen);
        return rlen;
    }

    bool complete() override {
        if (!checkHead(mHeadList, 2))return false;
        return (mDlen >= MIN_LEN) && (mDlen >= (mBuf[4] << 8 | mBuf[5]) + MIN_LEN);
    }

    bool check() override {
        mDataLen = (mBuf[4] << 8 | mBuf[5]) + MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    }

    int getType() override {
        return BT_TUYA;
    }
};

#endif // !__TUYA_PACKET_H__
