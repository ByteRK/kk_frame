/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:55:59
 * @FilePath: /hana_frame/src/protocol/packet_base.h
 * @Description: 串口数据包基类
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef __packet_base_h__
#define __packet_base_h__

#include "common.h"
#include "comm_func.h"

// Packet类型
typedef enum {
    BT_NULL = 0,
    BT_MCU,
    BT_BTN,
    BT_TUYA,
} BufferType;

#pragma pack(1)
// 数据缓冲区
typedef struct {
    short   type;   // 数据类型
    short   slen;   // 分配缓冲区大小
    short   len;    // 有效数据长度
    uint8_t buf[1]; // 缓冲区
} BuffData;
#pragma pack()

// 数据流出 UI -->
class IAsk {
public:
    BuffData* mBf;                                                     // 数据缓冲区
public:
    virtual void parse(BuffData* buf) {                                // 解析数据
        mBf = buf;
        if (mBf->len == 0)  mBf->len = mBf->slen;
    };
    virtual void setData(uint8_t pos, uint8_t data) {                  // 设置单个数据
        mBf->buf[pos] = data;
    }
    virtual void setData(uint8_t* data, uint8_t pos, uint8_t len) {    // 设置多个数据
        memcpy(mBf->buf + pos, data, len);
    }
    virtual void checkCode() = 0;                                      // 设置校验位
};

// 数据流入 UI <--
class IAck {
public:
    uint8_t* mBuf;             // 数据缓冲区
    short    mDlen;            // 处理的数据长度
    short*   mPlen;            // BuffData的长度指针（buf->len）
    uint16_t mDataLen;         // 真实数据长度
public:
    virtual void parse(BuffData* buf) {                        // 解析数据
        mBuf = buf->buf;
        mDlen = buf->len;
        mPlen = &buf->len;
    };
    virtual int  add(uint8_t* bf, int len) = 0;                // 添加数据
    virtual bool complete() = 0;                               // 数据是否完整
    virtual bool check() = 0;                                  // 校验数据
public:
    virtual int  getType() = 0;                                // 包类型
    virtual uint8_t getData(int pos) {                         // 获取单个数据
        if (pos >= 0 && pos < mDataLen - 1) { return mBuf[pos]; }
        return 0;
    };
    virtual uint16_t getData2(int pos, bool swap = false) {    // 获取两个数据
        if (pos >= 0 && pos + 1 < mDataLen - 1) {
            uint16_t result;
            if (swap)
                result = (static_cast<uint16_t>(mBuf[pos + 1]) << 8) | mBuf[pos];
            else
                result = (static_cast<uint16_t>(mBuf[pos]) << 8) | mBuf[pos + 1];
            return result;
        }
        return 0;
    };
protected:
    void addData(const uint16_t maxLen, uint8_t* data, int dataLen, int& rlen) {
        if (maxLen - mDlen >= dataLen - rlen) {
            memcpy(mBuf + mDlen, data + rlen, dataLen - rlen);
            mDlen += dataLen - rlen;
            rlen = dataLen;
        } else {
            memcpy(mBuf + mDlen, data + rlen, maxLen - mDlen);
            rlen += maxLen - mDlen;
            mDlen = maxLen;
        }
    };
    bool checkHead(const uint8_t* headList, int len) {
        if (mDlen < len) return false;
        for (int i = 0; i < len; i++)
            if (mBuf[i] != headList[i]) return false;
        return true;
    };
    bool findHead(const uint8_t* headList, int len) {
        int16_t i = 0, j = -1;
        bool result = false;
        if (mDlen < len) return result;
        for (; i + len <= mDlen; i++) {
            if (memcmp(mBuf + i, headList, len) == 0) {
                result = true;
                j = i;
                break;
            }
            if (i + len == mDlen)j = i;
        }
        if (j == -1) {
            mDlen = 0;
        } else if (j > 0) {
            memmove(mBuf, mBuf + j, mDlen - j);
            mDlen -= j;
        }
        return result;
    };
};

#endif // __packet_base_h__
