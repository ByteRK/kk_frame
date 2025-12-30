/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2025-12-29 17:48:39
 * @FilePath: /kk_frame/src/app/protocol/mcu_ui.h
 * @Description: 收发包定义
 * @BugList: 
 * 
 * Copyright (c) 2025 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __MCU_UI_H__
#define __MCU_UI_H__

#include "packet_base.h"
#include "proto.h"
#include "check_utils.h"
#include "string_utils.h"

#include <cdlog.h>

class BtnAsk :public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 18;
public:
    BtnAsk() { };
    BtnAsk(BuffData* buf) { parse(buf); }
    void parse(BuffData* buf)override {
        IAsk::parse(buf);
        mBf->buf[0] = 0xAA;
        mBf->buf[1] = 0x0E;
    }
    void checkCode()override {
        mBf->buf[mBf->len - 1] = CheckUtils::checkSum(mBf->buf, mBf->len - 1) & 0xFF;
    }
};

class McuAsk :public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 23;
public:
    McuAsk() { };
    McuAsk(BuffData* buf) { parse(buf); }
    void parse(BuffData* buf)override {
        IAsk::parse(buf);
        mBf->buf[0] = 0xA5;
        mBf->buf[1] = 0x5A;
    }
    void checkCode()override {
        mBf->buf[mBf->len - 1] = CheckUtils::checkSum(mBf->buf + 2, mBf->len - 3) & 0xFF;
    }
};

class TuyaAsk :public IAsk {
public:
    constexpr static uint16_t MIN_LEN = 7;
public:
    TuyaAsk() { };
    TuyaAsk(BuffData* buf) { parse(buf); }
    void parse(BuffData* buf)override {
        IAsk::parse(buf);
        mBf->buf[0] = 0x55;
        mBf->buf[1] = 0xAA;
    }
    void checkCode()override {
        mBf->buf[mBf->len - 1] = CheckUtils::checkSum(mBf->buf, mBf->len - 1) % 0x100;
    }
};

class BtnAck :public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0xFF;                 // 缓冲区大小
    constexpr static uint8_t  MIN_LEN = 9;                    // 接收数据最小长度
private:
    uint8_t  mHeadList[2] = { 0xAA, 0x12 };                   // 帧头列表
public:
    BtnAck() { };
    BtnAck(BuffData* buf) { parse(buf); }

    int add(uint8_t* bf, int len)override {
        int rlen = 0;
        while (mDlen < BUF_LEN && rlen < len) {
            addData(BUF_LEN, bf, len, rlen);
            if (!checkHead(mHeadList, 2))findHead(mHeadList, 2);
            *mPlen = mDlen;
        }
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << StringUtils::hexStr(mBuf, mDlen);
        return rlen;
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return mDlen >= MIN_LEN;
    };

    bool check()override {
        mDataLen = MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    };

    int getType()override {
        return BT_BTN;
    };
};


class McuAck :public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0xFF;                 // 缓冲区大小
    constexpr static uint8_t  MIN_LEN = 36;                   // 接收数据最小长度
private:
    uint8_t  mHeadList[2] = { 0xA5, 0x5A };                   // 帧头列表
public:
    McuAck() { };
    McuAck(BuffData* buf) { parse(buf); }

    int add(uint8_t* bf, int len)override {
        int rlen = 0;
        while (mDlen < BUF_LEN && rlen < len) {
            addData(BUF_LEN, bf, len, rlen);
            if (!checkHead(mHeadList, 2))findHead(mHeadList, 2);
            *mPlen = mDlen;
        }
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << StringUtils::hexStr(mBuf, mDlen);
        return rlen;
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return mDlen >= MIN_LEN;
    };

    bool check()override {
        mDataLen = MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    };

    int getType()override {
        return BT_MCU;
    };
};


class TuyaAck :public IAck {
public:
    constexpr static uint16_t BUF_LEN = 0x466;                // 缓冲区大小
    constexpr static uint8_t  MIN_LEN = 7;                    // 接收数据最小长度
private:
    uint8_t  mHeadList[2] = { 0x55, 0xAA };                   // 帧头列表
public:
    TuyaAck() { };
    TuyaAck(BuffData* buf) { parse(buf); }

    int add(uint8_t* bf, int len)override {
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
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return (mDlen >= MIN_LEN) && (mDlen >= (mBuf[4] << 8 | mBuf[5]) + MIN_LEN);
    };

    bool check()override {
        mDataLen = (mBuf[4] << 8 | mBuf[5]) + MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = CheckUtils::checkSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    };

    int getType()override {
        return BT_TUYA;
    };
};

#endif // !__MCU_UI_H__
