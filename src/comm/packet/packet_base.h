/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_base.h
 * @Description: 通讯数据包基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_BASE_H__
#define __PACKET_BASE_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#pragma pack(1)
/// @brief 收发数据包
typedef struct {
    short   type;    // 包类型
    short   len;     // 数据长度
    short   slen;    // 数据区容量
    uint8_t buf[1];  // 数据区指针
} BuffData;
#pragma pack()

/// @brief 发包编码器
class IAsk {
protected:
    BuffData* mBuf{ nullptr }; // 数据源

public:
    virtual ~IAsk();
    virtual void parse(BuffData* buf);
    virtual void setData(size_t pos, uint8_t data);
    virtual void setData(const uint8_t* data, size_t pos, size_t len);
    virtual void checkCode() = 0; // 校验位设定
};

/// @brief 收包解码器
class IAck {
public:
    uint8_t* mBuf{ nullptr };   // 接收缓存数据区首地址
    short*   mPlen{ nullptr };  // 数据包长度位指针(用于将有效长度同步到外部数据包)
    short    mDlen{ 0 };        // 接收缓存中当前已有的字节数
    uint16_t mDataLen{ 0 };     // 当前完整协议包总长度（含校验字节）

public:
    virtual ~IAck() { }

    /** @brief 绑定接收缓存，并同步当前有效长度。 */
    virtual void parse(BuffData* buf) {
        mBuf = buf->buf;
        mDlen = buf->len;
        mPlen = &buf->len;
    }

    /** @brief 追加输入字节，返回本次实际消费的输入长度。 */
    virtual int  add(const uint8_t* bf, int len) = 0;
    /** @brief 判断当前缓存是否已包含一个完整数据包。 */
    virtual bool complete() = 0;
    /** @brief 校验当前完整数据包是否合法。 */
    virtual bool check() = 0;
    /** @brief 返回当前数据包的业务命令类型。 */
    virtual int  getType() = 0;

    /** @brief 按偏移读取一个字节，越界时返回 0。 */
    virtual uint8_t getData(int pos) const {
        if (pos >= 0 && pos < mDataLen - 1) {
            return mBuf[pos];
        }
        return 0;
    }

    /** @brief 按偏移读取两个字节；swap 为 true 时按低字节在前组合。 */
    virtual uint16_t getData2(int pos, bool swap = false) const {
        if (pos >= 0 && pos + 1 < mDataLen - 1) {
            if (swap) {
                return (static_cast<uint16_t>(mBuf[pos + 1]) << 8) | mBuf[pos];
            }
            return (static_cast<uint16_t>(mBuf[pos]) << 8) | mBuf[pos + 1];
        }
        return 0;
    }

protected:
    void addData(const uint16_t maxLen, const uint8_t* data, int dataLen, int& rlen) {
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
