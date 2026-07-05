/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:45:36
 * @LastEditTime: 2026-07-05 22:36:12
 * @FilePath: /kk_frame/src/comm/packet/packet_buffer.cc
 * @Description: 通讯数据包缓存
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_buffer.h"

#include <cdlog.h>

#include <stdlib.h>

/// @brief 通讯数据包缓存构造
PacketBufferPool::PacketBufferPool() :
    mMaxCacheCount(DEFAULT_MAX_CACHE_COUNT) {
    mBuffs.reserve(mMaxCacheCount);
}

/// @brief 通讯数据包缓存析构
PacketBufferPool::~PacketBufferPool() {
    for (BuffData* bf : mBuffs) {
        free(bf);
    }
    mBuffs.clear();
}

/// @brief 设置最大缓存数量
/// @param count 最大缓存数量；设置为 0 表示不保留空闲缓存
void PacketBufferPool::setMaxCacheCount(size_t count) {
    mMaxCacheCount = count;
    if (count > mBuffs.capacity()) {
        mBuffs.reserve(count);
    }
    if (mBuffs.size() <= mMaxCacheCount) {
        return;
    }
    LOGW("Packet buffer cache exceeds limit. cached=%zu max=%zu",
        mBuffs.size(), mMaxCacheCount);
    while (mBuffs.size() > mMaxCacheCount) {
        free(mBuffs.back());
        mBuffs.pop_back();
    }
}

/// @brief 获取最大缓存数量
/// @return 最大缓存数量
size_t PacketBufferPool::maxCacheCount() const {
    return mMaxCacheCount;
}

/// @brief 回收数据包
/// @param buf 数据包
void PacketBufferPool::recycle(BuffData* buf) {
    if (buf == nullptr) {
        return;
    }
    if (mBuffs.size() >= mMaxCacheCount) {
        LOGW("Packet buffer cache exceeds limit. cached=%zu max=%zu",
            mBuffs.size() + 1, mMaxCacheCount);
        free(buf);
        return;
    }
    memset(buf->buf, 0, buf->slen);
    buf->len = 0;
    mBuffs.push_back(buf);
}

/// @brief 取得一个指定类型和容量的清空数据包
/// @param type 数据包类型
/// @param baseCapacity 协议基础长度
/// @param dataLen 在协议基础长度之外追加的可变数据长度
/// @return 优先复用类型和容量匹配的缓存，否则分配新缓存
BuffData* PacketBufferPool::obtainWithCapacity(int16_t type, uint16_t baseCapacity, uint16_t dataLen) {
    if (baseCapacity == 0 || dataLen > UINT16_MAX - baseCapacity) {
        LOGE("Packet buffer length out of range. type=%d base=%u data=%u max=%u",
            type, baseCapacity, dataLen, UINT16_MAX);
        return nullptr;
    }

    const uint16_t len = baseCapacity + dataLen;
    for (size_t i = 0; i < mBuffs.size(); ++i) {
        BuffData* bf = mBuffs[i];
        if (bf->type == type && bf->slen == len) {
            bf->len = 0;
            memset(bf->buf, 0, bf->slen);
            mBuffs[i] = mBuffs.back();
            mBuffs.pop_back();
            return bf;
        }
    }

    BuffData* bf = static_cast<BuffData*>(calloc(1, sizeof(BuffData) + len));
    if (bf == nullptr) {
        LOGE("Packet buffer allocation failed. type=%d len=%u", type, len);
        return nullptr;
    }
    bf->type = type;
    bf->slen = len;
    bf->len = 0;
    return bf;
}
