/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:45:36
 * @LastEditTime: 2026-07-05 20:59:39
 * @FilePath: /kk_frame/src/comm/packet/packet_buffer.cc
 * @Description: 通讯数据包缓存
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_buffer.h"

#include <iomanip>
#include <sstream>
#include <stdlib.h>

/// @brief 通讯数据包缓存构造
PacketBuffer::PacketBuffer() :
    mMaxCacheCount(DEFAULT_MAX_CACHE_COUNT) {
    mBuffs.reserve(mMaxCacheCount);
}

/// @brief 通讯数据包缓存析构
PacketBuffer::~PacketBuffer() {
    for (BuffData* bf : mBuffs) {
        free(bf);
    }
    mBuffs.clear();
    delete mSND; mSND = nullptr;
    delete mRCV; mRCV = nullptr;
}

/// @brief 设置最大缓存数量
/// @param count 最大缓存数量；设置为 0 表示不保留空闲缓存
void PacketBuffer::setMaxCacheCount(size_t count) {
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
size_t PacketBuffer::maxCacheCount() const {
    return mMaxCacheCount;
}

/// @brief 回收数据包
/// @param buf 数据包
void PacketBuffer::recycle(BuffData* buf) {
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

/// @brief 原始数据流传入
/// @param buf 数据包
/// @param inBuf 原始数据流
/// @param len 数据流长度
/// @return 真实消费数据长度
int PacketBuffer::add(BuffData* buf, const uint8_t* inBuf, int len) {
    if (buf == nullptr || inBuf == nullptr || len <= 0 || mRCV == nullptr) {
        return 0;
    }
    mRCV->parse(buf);
    return mRCV->add(inBuf, len);
}

/// @brief 检查数据包是否完整
/// @param buf 数据包
/// @return true 完整；false 不完整
bool PacketBuffer::complete(BuffData* buf) {
    if (buf == nullptr || mRCV == nullptr) {
        return false;
    }
    mRCV->parse(buf);
    return mRCV->complete();
}

/// @brief 比较数据包
/// @param src 源数据包
/// @param dst 目标数据包
/// @return true 相同；false 不同
bool PacketBuffer::compare(BuffData* src, BuffData* dst) {
    if (src == nullptr || dst == nullptr) {
        return false;
    }
    if (src->type != dst->type || src->len != dst->len) {
        return false;
    }
    return memcmp(src->buf, dst->buf, src->len) == 0;
}

/// @brief 校验数据包
/// @param buf 数据包
/// @return true 通过；false 不通过
bool PacketBuffer::check(BuffData* buf) {
    if (buf == nullptr || mRCV == nullptr) {
        return false;
    }
    mRCV->parse(buf);
    return mRCV->check();
}

/// @brief 绑定数据包并获取接收解码器
/// @param bf 数据包
/// @return 接收解码器，参数或解码器无效时返回 nullptr
IAck* PacketBuffer::ack(BuffData* bf) {
    if (bf == nullptr || mRCV == nullptr) {
        return nullptr;
    }
    mRCV->parse(bf);
    return mRCV;
}

/// @brief 设置数据包校验位
/// @param buf 数据包
void PacketBuffer::checkCode(BuffData* buf) {
    if (buf == nullptr || mSND == nullptr) {
        return;
    }
    mSND->parse(buf);
    mSND->checkCode();
}

/// @brief 数据包原始数据转换为字符串
/// @param buf 数据包
/// @return 数据包原始数据字符串
std::string PacketBuffer::str(BuffData* buf) {
    if (buf == nullptr) {
        return std::string();
    }
    std::ostringstream oss;
    for (int i = 0; i < buf->len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(buf->buf[i]) << " ";
    }
    return oss.str();
}
