
#ifndef __mcu_ui_h__
#define __mcu_ui_h__

#include "common.h"
#include "comm_func.h"

#include "packet_base.h"
#include "proto.h"

/// @brief 计算校验和
/// @param ptr 首地址
/// @param len 长度
/// @return 校验和
static uint64_t calculateCheckSum(uint8_t* ptr, uint16_t len) {
    uint64_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += ptr[i];
    }
    return sum;
};

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
        mBf->buf[mBf->len - 1] = calculateCheckSum(mBf->buf, mBf->len - 1) & 0xFF;
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
        mBf->buf[mBf->len - 1] = calculateCheckSum(mBf->buf + 2, mBf->len - 3) & 0xFF;
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
        mBf->buf[mBf->len - 1] = calculateCheckSum(mBf->buf, mBf->len - 1) % 0x100;
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
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << hexstr(mBuf, mDlen);
        return rlen;
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return mDlen >= MIN_LEN;
    };

    bool check()override {
        mDataLen = MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = calculateCheckSum(mBuf, mDataLen - 1) & 0xFF;
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
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << hexstr(mBuf, mDlen);
        return rlen;
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return mDlen >= MIN_LEN;
    };

    bool check()override {
        mDataLen = MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = calculateCheckSum(mBuf, mDataLen - 1) & 0xFF;
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
        LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << hexstr(mBuf, mDlen);
        return rlen;
    };

    bool complete()override {
        if (!checkHead(mHeadList, 2))return false;
        return (mDlen >= MIN_LEN) && (mDlen >= (mBuf[4] << 8 | mBuf[5]) + MIN_LEN);
    };

    bool check()override {
        mDataLen = (mBuf[4] << 8 | mBuf[5]) + MIN_LEN;
        uint8_t sum = mBuf[mDataLen - 1];
        uint8_t realSum = calculateCheckSum(mBuf, mDataLen - 1) & 0xFF;
        bool res = sum == realSum;
        LOGE_IF(!res, "[BTN] check error 0x%x -> 0x%x", realSum, sum);
        return res;
    };

    int getType()override {
        return BT_TUYA;
    };
};

#endif
