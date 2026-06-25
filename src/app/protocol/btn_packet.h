/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/app/protocol/btn_packet.h
 * @Description: Button protocol packets
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BTN_PACKET_H__
#define __BTN_PACKET_H__

#include "comm/packet/packet_base.h"
#include "proto.h"
#include "check_utils.h"
#include "string_utils.h"

#include <cdlog.h>

class BtnAsk : public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 18;

public:
    BtnAsk() { }
    BtnAsk(BuffData* buf) { parse(buf); }

    void parse(BuffData* buf) override {
        IAsk::parse(buf);
        mBf->buf[0] = 0xAA;
        mBf->buf[1] = 0x0E;
    }

    void checkCode() override {
        mBf->buf[mBf->len - 1] = CheckUtils::checkSum(mBf->buf, mBf->len - 1) & 0xFF;
    }
};

class BtnAck : public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0xFF;
    constexpr static uint8_t MIN_LEN = 9;

private:
    uint8_t mHeadList[2] = { 0xAA, 0x12 };

public:
    BtnAck() { }
    BtnAck(BuffData* buf) { parse(buf); }

    int add(uint8_t* bf, int len) override {
        int rlen = 0;
        while (mDlen < BUF_LEN && rlen < len) {
            addData(BUF_LEN, bf, len, rlen);
            if (!checkHead(mHeadList, 2))findHead(mHeadList, 2);
            *mPlen = mDlen;
        }
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << StringUtils::hexStr(mBuf, mDlen);
        return rlen;
    }

    bool complete() override {
        if (!checkHead(mHeadList, 2))return false;
        return mDlen >= MIN_LEN;
    }

    bool check() override {
        mDataLen = MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    }

    int getType() override {
        return BT_BTN;
    }
};

#endif // !__BTN_PACKET_H__
