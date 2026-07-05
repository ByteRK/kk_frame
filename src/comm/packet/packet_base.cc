/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:44:10
 * @LastEditTime: 2026-07-06 01:00:24
 * @FilePath: /kk_frame/src/comm/packet/packet_base.cc
 * @Description: 通讯数据包基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_base.h"

#include <cdlog.h>

IAsk::~IAsk() { }

/// @brief 数据源绑定
/// @param buf 数据源
void IAsk::parse(BuffData* buf) {
    mBuf = buf;
    if (mBuf->len == 0) {
        mBuf->len = mBuf->slen;
    }
}

/// @brief 数据设置
/// @param pos 位置
/// @param data 值
void IAsk::setData(size_t pos, uint8_t data) {
    if (mBuf == nullptr) {
        LOGE("IAsk setData failed: packet buffer is null");
        return;
    }
    if (mBuf->slen <= 0) {
        LOGE("IAsk setData failed: invalid capacity=%d", mBuf->slen);
        return;
    }
    if (pos >= static_cast<size_t>(mBuf->slen)) {
        LOGE("IAsk setData out of bounds. pos=%zu capacity=%d", pos, mBuf->slen);
        return;
    }
    mBuf->buf[pos] = data;
}

/// @brief 数据拷贝
/// @param data 数据指针
/// @param pos 目标起始位置
/// @param len 数据长度
/// @note 范围越界时不写入
void IAsk::setData(const uint8_t* data, size_t pos, size_t len) {
    if (len == 0) {
        return;
    }
    if (mBuf == nullptr) {
        LOGE("IAsk setData failed: packet buffer is null");
        return;
    }
    if (mBuf->slen <= 0) {
        LOGE("IAsk setData failed: invalid capacity=%d", mBuf->slen);
        return;
    }
    if (data == nullptr) {
        LOGE("IAsk setData failed: source data is null. pos=%zu len=%zu", pos, len);
        return;
    }

    const size_t capacity = static_cast<size_t>(mBuf->slen);
    if (pos > capacity || len > capacity - pos) {
        LOGE("IAsk setData out of bounds. pos=%zu len=%zu capacity=%zu",
            pos, len, capacity);
        return;
    }
    memcpy(mBuf->buf + pos, data, len);
}

/// @brief 绑定接收数据包
/// @param buf 数据包
void IAck::parse(BuffData* buf) {
    mPacket = buf;
    mBuf = buf == nullptr ? nullptr : buf->buf;
    updateDataLength();
}

/// @brief 原始数据流入口
/// @param data 数据指针
/// @param len 数据长度
/// @return 真实消费数据长度
/// @note 处理分为三个阶段：寻找并对齐包头、补齐长度字段、批量补齐完整包
/// @note 包头未对齐时按字节扫描；包头匹配后使用 memcpy 按目标长度批量复制
int IAck::add(const uint8_t* data, int len) {
    // 接收缓存、输入数据或容量无效时，本轮不消费任何数据
    if (mPacket == nullptr || mPacket->slen == 0 || data == nullptr || len <= 0) {
        return 0;
    }

    // 校验协议提供的包头长度及“可读取完整包长”所需的缓存长度
    const uint16_t headerSize = headLength();
    const uint16_t lengthReady = lengthReadySize();
    if (head() == nullptr || headerSize == 0 || headerSize > mPacket->slen
        || lengthReady < headerSize || lengthReady > mPacket->slen) {
        return 0;
    }

    // 当前缓存已经是完整包时等待上层处理，避免覆盖尚未分发的数据
    if (complete()) {
        return 0;
    }

    int consumed = 0;
    while (consumed < len) {
        // 阶段一：逐字节寻找包头。alignHead() 会丢弃无效前缀
        // 并保留末尾可能构成下一次包头的部分字节，以支持跨批次包头
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

        // 阶段二：包头已对齐，一次性补齐到协议能够读取完整包长的位置
        // 固定长度协议的 lengthReady 通常等于包头长度，因此不会额外复制
        copyInput(data, len, consumed, lengthReady);
        if (mPacket->len < lengthReady) {
            // 当前输入不足以读取完整包长，保留缓存等待下一批数据。
            break;
        }

        // 由具体协议解析完整包长，并确保该长度能被当前接收缓存容纳
        const int32_t expected = expectedLength();
        if (expected < lengthReady || expected > mPacket->slen) {
            // 长度字段非法：丢弃当前包头首字节，重新从已有数据中寻头
            discardPrefix(1);
            alignHead();
            continue;
        }

        // 阶段三：已知完整包长，批量复制本包剩余数据，不复制后续粘包
        copyInput(data, len, consumed, static_cast<uint16_t>(expected));
        if (mPacket->len == expected) {
            // 只记录第一个完整包；返回后上层会从 consumed 位置继续处理
            mDataLen = mPacket->len;
            break;
        }
    }
    return consumed;
}

/// @brief 数据包完整性校验
/// @return 数据包是否完整
bool IAck::complete() {
    updateDataLength();
    return mDataLen > 0;
}

/// @brief 获取数据包原始指针
/// @return 数据指针
const uint8_t* IAck::data() const {
    return mBuf;
}

/// @brief 获取数据包长度
/// @return 数据长度
uint16_t IAck::dataLength() const {
    return mDataLen;
}

/// @brief 判断完整数据包内是否包含指定范围
/// @param offset 起始偏移
/// @param length 数据长度
/// @return true 范围有效，false 范围越界或当前无完整数据包
bool IAck::hasRange(size_t offset, size_t length) const {
    return mBuf != nullptr
        && offset <= mDataLen
        && length <= static_cast<size_t>(mDataLen) - offset;
}

/// @brief 读取无符号单字节整数
/// @param offset 起始偏移
/// @param value 读取结果
/// @return true 成功
bool IAck::readU8(size_t offset, uint8_t& value) const {
    if (!hasRange(offset, sizeof(value))) return false;
    value = mBuf[offset];
    return true;
}

/// @brief 读取无符号双字节整数
/// @param offset 起始偏移
/// @param value 读取结果
/// @param order 大小端序
/// @return true 成功
bool IAck::readU16(size_t offset, uint16_t& value, ByteOrder order) const {
    if (!hasRange(offset, sizeof(value))) return false;
    if (order == ByteOrder::BigEndian) {
        value = (static_cast<uint16_t>(mBuf[offset]) << 8)
            | static_cast<uint16_t>(mBuf[offset + 1]);
    } else {
        value = (static_cast<uint16_t>(mBuf[offset + 1]) << 8)
            | static_cast<uint16_t>(mBuf[offset]);
    }
    return true;
}

/// @brief 读取无符号四字节整数
/// @param offset 起始偏移
/// @param value 读取结果
/// @param order 大小端序
/// @return true 成功
bool IAck::readU32(size_t offset, uint32_t& value, ByteOrder order) const {
    if (!hasRange(offset, sizeof(value))) return false;
    value = 0;
    if (order == ByteOrder::BigEndian) {
        for (size_t i = 0; i < sizeof(value); ++i) {
            value = (value << 8) | static_cast<uint32_t>(mBuf[offset + i]);
        }
    } else {
        for (size_t i = 0; i < sizeof(value); ++i) {
            value |= static_cast<uint32_t>(mBuf[offset + i]) << (i * 8);
        }
    }
    return true;
}

/// @brief 读取无符号八字节整数
/// @param offset 起始偏移
/// @param value 读取结果
/// @param order 大小端序
/// @return true 成功
bool IAck::readU64(size_t offset, uint64_t& value, ByteOrder order) const {
    if (!hasRange(offset, sizeof(value))) return false;
    value = 0;
    if (order == ByteOrder::BigEndian) {
        for (size_t i = 0; i < sizeof(value); ++i) {
            value = (value << 8) | static_cast<uint64_t>(mBuf[offset + i]);
        }
    } else {
        for (size_t i = 0; i < sizeof(value); ++i) {
            value |= static_cast<uint64_t>(mBuf[offset + i]) << (i * 8);
        }
    }
    return true;
}

/// @brief 获取完整数据包内指定范围的只读视图
/// @param offset 起始偏移
/// @param length 数据长度
/// @param bytes 数据指针
/// @return true 成功读取
/// @note 返回指针由 IAck 绑定的缓存持有，仅在当前数据包回调期间有效
bool IAck::readBytes(size_t offset, size_t length, const uint8_t*& bytes) const {
    if (!hasRange(offset, length)) {
        bytes = nullptr;
        return false;
    }
    bytes = mBuf + offset;
    return true;
}

/// @brief 判断包头是否已对齐
/// @return true 已对齐
bool IAck::hasHead() const {
    const uint16_t length = headLength();
    return mPacket != nullptr && head() != nullptr && length > 0
        && mPacket->len >= length && memcmp(mBuf, head(), length) == 0;
}

/// @brief 尝试对齐包头
/// @return true 包头已对齐
/// @note 丢弃缓存中不匹配包头的前缀部分，并保留可能构成下一次包头的部分字节
bool IAck::alignHead() {
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

/// @brief 丢弃缓存中指定长度的数据
/// @param length 丢弃长度
void IAck::discardPrefix(uint16_t length) {
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

/// @brief 从输入流批量复制数据
/// @param data 输入数据
/// @param dataLen 输入数据总长度
/// @param consumed 已消耗长度
/// @param targetLen 目标长度
void IAck::copyInput(const uint8_t* data, int dataLen, int& consumed, uint16_t targetLen) {
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

/// @brief 根据协议更新真实数据包长度
void IAck::updateDataLength() {
    mDataLen = 0;
    if (!hasHead() || mPacket->len < lengthReadySize()) {
        return;
    }
    const int32_t expected = expectedLength();
    if (expected > 0 && expected <= mPacket->slen && mPacket->len >= expected) {
        mDataLen = static_cast<uint16_t>(expected);
    }
}
