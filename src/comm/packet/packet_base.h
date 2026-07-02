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
    int16_t  type;    // 包类型
    uint16_t len;     // 数据长度
    uint16_t slen;    // 数据区容量
    uint8_t  buf[1];  // 数据区指针
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
    uint8_t*  mBuf{ nullptr };    // 接收缓存数据区首地址
    uint16_t  mDataLen{ 0 };      // 当前完整协议包总长度（含校验字节）

public:
    virtual ~IAck() { }
    /** @brief 校验当前完整数据包是否合法。 */
    virtual bool check() = 0;
    /** @brief 返回当前数据包的业务命令类型。 */
    virtual int getType() = 0;

    /** @brief 绑定接收缓存。 */
    void parse(BuffData* buf) {
        mPacket = buf;
        mBuf = buf == nullptr ? nullptr : buf->buf;
        updateDataLength();
    }

    /**
     * @brief 将输入流追加到接收缓存，遇到第一个完整包时停止。
     * @param data 本次输入数据。
     * @param len 本次输入长度。
     * @return 实际消费的输入长度；未消费部分由上层继续送入下一轮解析。
     *
     * 处理分为三个阶段：寻找并对齐包头、补齐长度字段、批量补齐完整包。
     * 包头未对齐时按字节扫描；包头匹配后使用 memcpy 按目标长度批量复制。
     */
    int add(const uint8_t* data, int len) {
        // 接收缓存、输入数据或容量无效时，本轮不消费任何数据。
        if (mPacket == nullptr || mPacket->slen == 0 || data == nullptr || len <= 0) {
            return 0;
        }

        // 校验协议提供的包头长度及“可读取完整包长”所需的缓存长度。
        const uint16_t headerSize = headLength();
        const uint16_t lengthReady = lengthReadySize();
        if (head() == nullptr || headerSize == 0 || headerSize > mPacket->slen
            || lengthReady < headerSize || lengthReady > mPacket->slen) {
            return 0;
        }

        // 当前缓存已经是完整包时等待上层处理，避免覆盖尚未分发的数据。
        if (complete()) {
            return 0;
        }

        int consumed = 0;
        while (consumed < len) {
            // 阶段一：逐字节寻找包头。alignHead() 会丢弃无效前缀，
            // 并保留末尾可能构成下一次包头的部分字节，以支持跨批次包头。
            while (consumed < len && !hasHead()) {
                if (mPacket->len >= mPacket->slen) {
                    mPacket->len = 0;
                }
                mBuf[mPacket->len++] = data[consumed++];
                alignHead();
            }
            if (!hasHead()) {
                break;
            }

            // 阶段二：包头已对齐，一次性补齐到协议能够读取完整包长的位置。
            // 固定长度协议的 lengthReady 通常等于包头长度，因此不会额外复制。
            copyInput(data, len, consumed, lengthReady);
            if (mPacket->len < lengthReady) {
                // 当前输入不足以读取完整包长，保留缓存等待下一批数据。
                break;
            }

            // 由具体协议解析完整包长，并确保该长度能被当前接收缓存容纳。
            const int32_t expected = expectedLength();
            if (expected < lengthReady || expected > mPacket->slen) {
                // 长度字段非法：丢弃当前包头首字节，重新从已有数据中寻头。
                discardPrefix(1);
                alignHead();
                continue;
            }

            // 阶段三：已知完整包长，批量复制本包剩余数据，不复制后续粘包。
            copyInput(data, len, consumed, static_cast<uint16_t>(expected));
            if (mPacket->len == expected) {
                // 只记录第一个完整包；返回后上层会从 consumed 位置继续处理。
                mDataLen = mPacket->len;
                break;
            }
        }
        return consumed;
    }

    /** @brief 判断当前缓存是否已包含一个完整数据包。 */
    bool complete() {
        updateDataLength();
        return mDataLen > 0;
    }

    /** @brief 按偏移读取一个字节，越界时返回 0。 */
    uint8_t getData(int pos) const {
        if (pos >= 0 && pos < mDataLen - 1) {
            return mBuf[pos];
        }
        return 0;
    }

    /** @brief 按偏移读取两个字节；swap 为 true 时按低字节在前组合。 */
    uint16_t getData2(int pos, bool swap = false) const {
        if (pos >= 0 && pos + 1 < mDataLen - 1) {
            if (swap) {
                return (static_cast<uint16_t>(mBuf[pos + 1]) << 8) | mBuf[pos];
            }
            return (static_cast<uint16_t>(mBuf[pos]) << 8) | mBuf[pos + 1];
        }
        return 0;
    }

private:
    /** @brief 返回协议包头。 */
    virtual const uint8_t* head() const = 0;
    /** @brief 返回协议包头长度。 */
    virtual uint16_t headLength() const = 0;
    /** @brief 返回能够读取完整包长时所需的最小缓存长度。 */
    virtual uint16_t lengthReadySize() const = 0;
    /** @brief 从已满足 lengthReadySize() 的缓存中返回完整包长。 */
    virtual int32_t expectedLength() const = 0;

    BuffData* mPacket{ nullptr };

    /** @brief 判断当前缓存是否已经以完整协议包头开头。 */
    bool hasHead() const {
        const uint16_t length = headLength();
        return mPacket != nullptr && head() != nullptr && length > 0
            && mPacket->len >= length && memcmp(mBuf, head(), length) == 0;
    }

    /**
     * @brief 在当前缓存中寻找包头并将其移动到数据区起始位置。
     * @return 已找到完整包头返回 true，否则返回 false。
     * @note 未找到时只保留末尾与包头前缀匹配的字节，用于处理跨批次包头。
     */
    bool alignHead() {
        if (mPacket == nullptr || head() == nullptr || headLength() == 0) {
            return false;
        }

        // 优先查找缓存中第一个完整包头，并丢弃它之前的噪声数据。
        const uint16_t length = headLength();
        for (uint16_t pos = 0; pos + length <= mPacket->len; ++pos) {
            if (memcmp(mBuf + pos, head(), length) == 0) {
                discardPrefix(pos);
                return true;
            }
        }

        // 没有完整包头时，计算缓存末尾与包头前缀匹配的最长长度。
        uint16_t keep = mPacket->len < length - 1 ? mPacket->len : length - 1;
        while (keep > 0) {
            if (memcmp(mBuf + mPacket->len - keep, head(), keep) == 0) {
                break;
            }
            --keep;
        }
        if (keep < mPacket->len) {
            memmove(mBuf, mBuf + mPacket->len - keep, keep);
        }
        mPacket->len = keep;
        return false;
    }

    /** @brief 从缓存头部丢弃指定长度，并将剩余数据前移。 */
    void discardPrefix(uint16_t length) {
        if (mPacket == nullptr || length == 0) {
            return;
        }
        if (length >= mPacket->len) {
            mPacket->len = 0;
            return;
        }
        memmove(mBuf, mBuf + length, mPacket->len - length);
        mPacket->len -= length;
    }

    /**
     * @brief 从输入流批量复制数据，最多将接收缓存补齐到 targetLen。
     * @param consumed 输入输出参数，调用后累加本次实际复制长度。
     * @note 函数不会复制超过当前包目标长度的数据，因此粘包数据保持未消费状态。
     */
    void copyInput(const uint8_t* data, int dataLen, int& consumed, uint16_t targetLen) {
        if (mPacket->len >= targetLen || consumed >= dataLen) {
            return;
        }
        const uint16_t need = targetLen - mPacket->len;
        const int remaining = dataLen - consumed;
        const uint16_t copyLen = need < remaining ? need : static_cast<uint16_t>(remaining);
        memcpy(mBuf + mPacket->len, data + consumed, copyLen);
        mPacket->len += copyLen;
        consumed += copyLen;
    }

    /** @brief 根据当前缓存和协议长度字段刷新完整包长度。 */
    void updateDataLength() {
        mDataLen = 0;
        if (!hasHead() || mPacket->len < lengthReadySize()) {
            return;
        }
        const int32_t expected = expectedLength();
        if (expected > 0 && expected <= mPacket->slen && mPacket->len >= expected) {
            mDataLen = static_cast<uint16_t>(expected);
        }
    }
};

#endif // !__PACKET_BASE_H__
