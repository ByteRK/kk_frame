/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_base.h
 * @Description: Application packet base
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_BASE_H__
#define __PACKET_BASE_H__

#include <stdint.h>
#include <string.h>

#pragma pack(1)
typedef struct {
    short   type;
    short   slen;
    short   len;
    uint8_t buf[1];
} BuffData;
#pragma pack()

class IAsk {
public:
    virtual ~IAsk() { }

    virtual void parse(BuffData* buf) {
        mBf = buf;
        if (mBf->len == 0) {
            mBf->len = mBf->slen;
        }
    }

    virtual void setData(uint8_t pos, uint8_t data) {
        mBf->buf[pos] = data;
    }

    virtual void setData(uint8_t* data, uint8_t pos, uint8_t len) {
        if (data && len > 0) {
            memcpy(mBf->buf + pos, data, len);
        }
    }

    virtual void checkCode() = 0;

protected:
    BuffData* mBf{ nullptr };
};

class IAck {
public:
    virtual ~IAck() { }

    virtual void parse(BuffData* buf) {
        mBuf = buf->buf;
        mDlen = buf->len;
        mPlen = &buf->len;
    }

    virtual int  add(uint8_t* bf, int len) = 0;
    virtual bool complete() = 0;
    virtual bool check() = 0;
    virtual int  getType() = 0;

    virtual uint8_t getData(int pos) {
        if (pos >= 0 && pos < mDataLen - 1) {
            return mBuf[pos];
        }
        return 0;
    }

    virtual uint16_t getData2(int pos, bool swap = false) {
        if (pos >= 0 && pos + 1 < mDataLen - 1) {
            if (swap) {
                return (static_cast<uint16_t>(mBuf[pos + 1]) << 8) | mBuf[pos];
            }
            return (static_cast<uint16_t>(mBuf[pos]) << 8) | mBuf[pos + 1];
        }
        return 0;
    }

public:
    uint8_t* mBuf{ nullptr };
    short    mDlen{ 0 };
    short*   mPlen{ nullptr };
    uint16_t mDataLen{ 0 };

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
    }

    bool checkHead(const uint8_t* headList, int len) {
        if (mDlen < len) {
            return false;
        }
        for (int i = 0; i < len; i++) {
            if (mBuf[i] != headList[i]) {
                return false;
            }
        }
        return true;
    }

    bool findHead(const uint8_t* headList, int len) {
        int16_t i = 0;
        int16_t j = -1;
        bool result = false;
        if (mDlen < len) {
            return result;
        }
        for (; i + len <= mDlen; i++) {
            if (memcmp(mBuf + i, headList, len) == 0) {
                result = true;
                j = i;
                break;
            }
            if (i + len == mDlen) {
                j = i;
            }
        }
        if (j == -1) {
            mDlen = 0;
        } else if (j > 0) {
            memmove(mBuf, mBuf + j, mDlen - j);
            mDlen -= j;
        }
        return result;
    }
};

#endif // !__PACKET_BASE_H__
