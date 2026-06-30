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
/** @brief 数据包缓存头；实际分配空间由头部和可变长 buf 区域组成。 */
typedef struct {
    /** @brief 协议类型，用于选择对应的数据包解析器。 */
    short   type;
    /** @brief buf 区域的总容量。 */
    short   slen;
    /** @brief buf 区域中当前有效的数据长度。 */
    short   len;
    /** @brief 可变长数据区的首字节。 */
    uint8_t buf[1];
} BuffData;
#pragma pack()

/**
 * @brief 发送包编码接口。
 *
 * parse() 绑定待编码的 BuffData，派生类通过 setData() 填充字段并实现 checkCode()
 * 写入协议校验值。IAsk 不拥有 BuffData。
 */
class IAsk {
public:
    virtual ~IAsk() { }

    /** @brief 绑定待编码缓存；首次绑定时默认将有效长度设为缓存容量。 */
    virtual void parse(BuffData* buf) {
        mBf = buf;
        if (mBf->len == 0) {
            mBf->len = mBf->slen;
        }
    }

    /** @brief 在数据区指定偏移写入一个字节。调用前必须已通过 parse() 绑定缓存。 */
    virtual void setData(uint8_t pos, uint8_t data) {
        mBf->buf[pos] = data;
    }

    /** @brief 从 data 复制 len 字节到数据区指定偏移。 */
    virtual void setData(uint8_t* data, uint8_t pos, uint8_t len) {
        if (data && len > 0) {
            memcpy(mBf->buf + pos, data, len);
        }
    }

    /** @brief 按具体协议计算并写入发送包校验值。 */
    virtual void checkCode() = 0;

protected:
    BuffData* mBf{ nullptr };
};

/**
 * @brief 接收包流式解析接口。
 *
 * 派生类定义包头定位、完整包判定、校验和命令类型提取。parse() 只绑定外部缓存，
 * IAck 不拥有 BuffData；返回给业务层的 IAck 也只应在当前分发期间使用。
 */
class IAck {
public:
    virtual ~IAck() { }

    /** @brief 绑定接收缓存，并同步当前有效长度。 */
    virtual void parse(BuffData* buf) {
        mBuf = buf->buf;
        mDlen = buf->len;
        mPlen = &buf->len;
    }

    /** @brief 追加输入字节，返回本次实际消费的输入长度。 */
    virtual int  add(uint8_t* bf, int len) = 0;
    /** @brief 判断当前缓存是否已包含一个完整数据包。 */
    virtual bool complete() = 0;
    /** @brief 校验当前完整数据包是否合法。 */
    virtual bool check() = 0;
    /** @brief 返回当前数据包的业务命令类型。 */
    virtual int  getType() = 0;

    /** @brief 按偏移读取一个字节，越界时返回 0。 */
    virtual uint8_t getData(int pos) {
        if (pos >= 0 && pos < mDataLen - 1) {
            return mBuf[pos];
        }
        return 0;
    }

    /** @brief 按偏移读取两个字节；swap 为 true 时按低字节在前组合。 */
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
