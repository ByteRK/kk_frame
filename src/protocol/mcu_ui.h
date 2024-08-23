
#ifndef __mcu_ui_ack_h__
#define __mcu_ui_ack_h__

#include <common.h>
#include <comm_func.h>

#include "ipacket_buffer.h"
#include "proto.h"

// 帧头
constexpr static uchar MCU_HEAD_SEND[2] = { 0x55, 0x15 };    // 电控板  发送
constexpr static uchar MCU_HEAD_RESP[1] = { 0x55 };          // 电控板  接收
constexpr static uchar FILM_HEAD_SEND[2] = { 0xAA, 0x0E };   // 彩膜按键 发送
constexpr static uchar FILM_HEAD_RESP[2] = { 0xBB, 0x09 };   // 彩膜按键 接收
constexpr static uchar TUYA_HEAD_SEND[2] = { 0x55, 0xAA };   // 涂鸦模组 发送
constexpr static uchar TUYA_HEAD_RESP[2] = { 0x55, 0xAA };   // 涂鸦模组 接收

// 固定数据位总长度 (定长为总长，不定长为帧头+帧尾+校验)
#define MCU_LEN_SEND   22
#define MCU_LEN_RESP   21
#define FILM_LEN_SEND  14
#define FILM_LEN_RESP  9
#define TUYA_LEN_SEND  7       // 不定长
#define TUYA_LEN_RESP  7       // 不定长

class DataCheck {
public:
    DataCheck() { }
    static unsigned char CRC8(unsigned char* ptr, unsigned short len) {
        unsigned char crc;
        unsigned char i;
        crc = 0;
        while (len--) {
            crc ^= *ptr++;
            for (i = 0; i < 8; i++) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x07;
                } else
                    crc <<= 1;
            }
        }
        return crc;
    }
    static unsigned int Sum(unsigned char* ptr, unsigned short len) {
        unsigned int sum = 0;
        for (int i = 0; i < len; i++) {
            sum += ptr[i];
        }
        return sum;
    }
    static unsigned char RebellionSumAdd1(unsigned char* ptr, unsigned short len) {
        return (~Sum(ptr, len) + 1) & 0xFF;
    }
    static unsigned char CheckMCU(unsigned char* ptr, unsigned short len) {
        return Sum(ptr + 1, len - 2) & 0xFF;
    }
    static unsigned char CheckBTN(unsigned char* ptr, unsigned short len) {
        return Sum(ptr, len - 1) & 0xFF;
    }
    static unsigned char CheckTUYA(unsigned char* ptr, unsigned short len) {
        return Sum(ptr, len - 1) % 0x100;
    }
};

// UI -> MCU
class UI2MCU : public IAsk, public DataCheck {
public:
    constexpr static uchar BUF_LEN = 0xFF;

public:
    int mType = BT_MCU;                          // 判断这个包来自哪里
    UI2MCU() { }
    UI2MCU(BuffData* buf) { parse(buf); }
    UI2MCU(BuffData* buf, int type) { setType(type); parse(buf); }
    void parse(BuffData* buf) {
        mBf = buf;
        if (mBf->len == 0) {
            mBf->len = mBf->slen;
            switch (mType) {
            case BT_MCU:
                mBf->buf[0] = MCU_HEAD_SEND[0];
                mBf->buf[1] = MCU_HEAD_SEND[1];
                break;
            case BT_BTN:
                mBf->buf[0] = FILM_HEAD_SEND[0];
                mBf->buf[1] = FILM_HEAD_SEND[1];
                break;
            case BT_TUYA:
                mBf->buf[0] = TUYA_HEAD_SEND[0];
                mBf->buf[1] = TUYA_HEAD_SEND[1];
                break;
            }
        }
        LOG(VERBOSE) << "parse=" << hexstr(mBf->buf, mBf->len);
    }

protected:
    virtual int getType() { return mType; }
    virtual int getCMD() { return 0; }

public:
    void setType(int type) { mType = type; }

    uchar getData(uchar pos) { return mBf->buf[pos]; };

    void setData(uchar pos, uchar data) { mBf->buf[pos] = data; }
    void setData(uchar* data, uchar pos, uchar len) { memcpy(mBf->buf + pos, data, len); }

    void checkcode() {
        switch (mType) {
        case BT_MCU:
            mBf->buf[mBf->len - 1] = CheckMCU(mBf->buf, mBf->len);
            break;
        case BT_BTN:
            mBf->buf[mBf->len - 1] = CheckBTN(mBf->buf, mBf->len);
            break;
        case BT_TUYA:
            mBf->buf[mBf->len - 1] = CheckTUYA(mBf->buf, mBf->len);
            break;
        }
    }
};

// MCU -> UI
class MCU2UI : public IAck, public DataCheck {
public:
    constexpr static ushort  BUF_LEN = 0x300;     // 缓冲区大小
    u_int16_t mDataLen;         // 数据长度
    bool FindHead;              // 是否已找到包头
    int mType = BT_MCU;         // 判断这个包来自哪里
    int mCMD = BT_MCU;          // 判断分配给哪个函数进行处理

public:
    MCU2UI() { }
    MCU2UI(BuffData* bf) { parse(bf); }

    /// @brief 构造
    /// @param bf 
    void parse(BuffData* bf) {
        mDlen = bf->len;
        mPlen = &bf->len;
        mBuf = bf->buf;
    }

    /// @brief 接收数据
    /// @param bf 
    /// @param len 
    /// @return 
    int add(uchar* bf, int len) {
        LOGV("接收数据");
        int  i, j, rlen = 0;
        if (mDlen == 0) FindHead = false;

        while (mDlen < BUF_LEN && rlen < len) {
            // add data
            if (BUF_LEN - mDlen >= len - rlen) {
                memcpy(mBuf + mDlen, bf + rlen, len - rlen);
                mDlen += len - rlen;
                rlen = len;
            } else {
                memcpy(mBuf + mDlen, bf + rlen, BUF_LEN - mDlen);
                rlen += BUF_LEN - mDlen;
                mDlen = BUF_LEN;
            }
            // find head
            int headLen = getHeadLen();
            if (!FindHead && mDlen > headLen) {
                for (i = 0, j = -1; i < mDlen; i++) {
                    if (mBuf[i] == getHeadIn(0)) {
                        if (headLen <= 1 || (i < mDlen - 1 && mBuf[i + 1] == getHeadIn(1))) {
                            j = i;
                            LOGV("FIND HEAD AT:%d", i);
                            FindHead = true;
                            break;
                        } else {
                            if (i == mDlen - 1) { j = i; }
                        }
                    }
                }
                if (j == -1) {
                    LOGV("NO HEAD!!!");
                    mDlen = 0;
                } else if (j > 0) {
                    memmove(mBuf, mBuf + j, mDlen - j);
                    mDlen -= j;
                }
            }
            *mPlen = mDlen;
        }

        if (mType == BT_TUYA)
            LOG(VERBOSE) << "mDlen:" << mDlen << " Bytes=" << hexstr(mBuf, mDlen);
        return rlen;
    }

    /// @brief 检查数据长度完整性
    /// @return 
    bool complete() {
        if (!FindHead)return false;
        switch (mType) {
        case BT_MCU:
            if (mDlen >= MCU_LEN_RESP &&
                mBuf[0] == MCU_HEAD_RESP[0]) {
                LOGV("The package has been received");
                return true;
            }
            break;
        case BT_BTN:
            if (mDlen >= FILM_LEN_RESP &&
                mBuf[0] == FILM_HEAD_RESP[0] && mBuf[1] == FILM_HEAD_RESP[1]) {
                LOGV("The package has been received");
                return true;
            }
            break;
        case BT_TUYA:
            if (mDlen >= TUYA_LEN_RESP &&
                mDlen >= (mBuf[4] << 8 | mBuf[5]) + TUYA_LEN_RESP &&
                mBuf[0] == TUYA_HEAD_RESP[0] && mBuf[1] == TUYA_HEAD_RESP[1]) {
                LOGV("The package has been received");
                return true;
            }
            break;
        }
        LOGV("Package not received yet");
        return false;
    }

    /// @brief 检查数据是否正确
    /// @return 
    bool check() {
        uchar sum = 0x00, realSum = 0x01;

        switch (mType) {
        case BT_MCU:
            mDataLen = MCU_LEN_RESP;
            sum = mBuf[mDataLen - 1];
            realSum = CheckMCU(mBuf, mDataLen);
            break;
        case BT_BTN:
            mDataLen = FILM_LEN_RESP;
            sum = mBuf[mDataLen - 1];
            realSum = CheckBTN(mBuf, mDataLen);
            break;
        case BT_TUYA:
            mDataLen = (mBuf[4] << 8 | mBuf[5]) + TUYA_LEN_RESP;
            sum = mBuf[mDataLen - 1];
            realSum = CheckTUYA(mBuf, mDataLen);
            break;
        }

        bool res = sum == realSum;
        LOGE_IF(!res, "[%d] check error 0x%x -> 0x%x", mType, realSum, sum);
        return res;
    }

protected:
    virtual int getType() { return mType; }
    virtual int getCMD() { return mCMD; }

    /// @brief 获取帧头长度
    /// @return 
    int getHeadLen() {
        switch (mType) {
        case BT_MCU:
            return sizeof(MCU_HEAD_RESP);
        case BT_BTN:
            return sizeof(FILM_HEAD_RESP);
        case BT_TUYA:
            return sizeof(TUYA_HEAD_RESP);
        }
        return 2;
    }

    /// @brief 获取帧头数据
    /// @param index 
    /// @return 
    int getHeadIn(int index) {
        switch (mType) {
        case BT_MCU:
            return MCU_HEAD_RESP[index];
        case BT_BTN:
            return FILM_HEAD_RESP[index];
        case BT_TUYA:
            return TUYA_HEAD_RESP[index];
        }
        return 0;
    }

public:
    void setCMD(int cmd) { mCMD = cmd; }
    void setType(int type) { mType = type; }

    int getData(int pos) {
        if (pos >= 0 && pos < mDataLen - 1) { return mBuf[pos]; }
        return 0;
    }
    int getData2(int pos, bool swap = false) {
        if (pos >= 0 && pos + 1 < mDataLen - 1) {
            ushort result;
            if (swap)
                result = (static_cast<ushort>(mBuf[pos + 1]) << 8) | mBuf[pos];
            else
                result = (static_cast<ushort>(mBuf[pos]) << 8) | mBuf[pos + 1];
            return result;
        }
        return 0;
    }
};

#endif
