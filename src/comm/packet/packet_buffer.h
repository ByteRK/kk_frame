/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:45:36
 * @LastEditTime: 2026-07-05 22:36:56
 * @FilePath: /kk_frame/src/comm/packet/packet_buffer.h
 * @Description: 通讯数据包缓存
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_BUFFER_H__
#define __PACKET_BUFFER_H__

#include "packet_base.h"

#include <memory>
#include <vector>

/// @brief 通讯数据包缓存池
/// @note 负责协议数据包的分配、复用，并为接收流创建独立解码器
/// @note obtainSend/obtainReceive 返回的数据包由外部持有，调用 recycle 回收
class PacketBufferPool {
public:
    static constexpr size_t DEFAULT_MAX_CACHE_COUNT = 16; // 默认最大数据包缓存数量

private:
    size_t                 mMaxCacheCount;  // 最大缓存数量
    std::vector<BuffData*> mBuffs;          // 数据包缓存池

public:
    PacketBufferPool();
    virtual ~PacketBufferPool();

    PacketBufferPool(const PacketBufferPool&) = delete;
    PacketBufferPool& operator=(const PacketBufferPool&) = delete;

    void    setMaxCacheCount(size_t count);
    size_t  maxCacheCount() const;

public: // 数据包缓存池
    /// @brief 回收数据包
    /// @param buf 数据包
    void recycle(BuffData* buf);

    /// @brief 取得一个清空后的发送数据包
    /// @param dataLen 在协议基础长度之外追加的可变数据长度
    /// @return 数据包
    virtual BuffData* obtainSend(uint16_t dataLen = 0) = 0;

    /// @brief 取得一个清空后的接收数据包
    /// @return 容量为协议接收上限的数据包
    virtual BuffData* obtainReceive() = 0;

    /// @brief 创建一个独立的接收解码器
    /// @return 解码器独占指针
    virtual std::unique_ptr<IAck> createAck() const = 0;

protected:
    BuffData* obtainWithCapacity(int16_t type, uint16_t baseCapacity, uint16_t dataLen = 0);
};

/// @brief 绑定具体协议类型、发送包类型和接收包类型的通讯数据包缓存实现
/// @tparam Ask IAsk 的具体发送编码类型，需提供 BASE_LEN
/// @tparam Ack IAck 的具体接收解析类型，需提供 BUFFER_CAPACITY
/// @tparam T 协议类型
template <int16_t T, typename Ask, typename Ack>
class PacketBufferPoolT : public PacketBufferPool {
public:
    /// @brief 取得一个清空后的发送数据包
    /// @param dataLen 在协议基础长度之外追加的可变数据长度
    /// @return 数据包
    BuffData* obtainSend(uint16_t dataLen = 0) override {
        return obtainWithCapacity(T, Ask::BASE_LEN, dataLen);
    }

    /// @brief 取得一个清空后的接收数据包
    /// @return 容量为协议接收上限的数据包
    BuffData* obtainReceive() override {
        return obtainWithCapacity(T, Ack::BUFFER_CAPACITY);
    }

    /// @brief 创建一个独立的接收解码器
    /// @return 解码器独占指针
    std::unique_ptr<IAck> createAck() const override {
        return std::unique_ptr<IAck>(new Ack());
    }
};

#endif // !__PACKET_BUFFER_H__
