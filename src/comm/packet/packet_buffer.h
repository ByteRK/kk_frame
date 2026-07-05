/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:45:36
 * @LastEditTime: 2026-07-05 20:59:48
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

#include <cdlog.h>

#include <stdlib.h>
#include <string>
#include <vector>

/// @brief 通讯数据包缓存
/// @note 提供缓存池+数据包编解码器
/// @note 内部持有并负责销毁编码器 mSND、解码器 mRCV 和数据包缓存
/// @note obtain 返回的数据包由外部持有，调用 recycle 回收
class PacketBuffer {
public:
    static constexpr size_t DEFAULT_MAX_CACHE_COUNT = 16; // 默认最大数据包缓存数量

protected:
    size_t                 mMaxCacheCount;  // 最大缓存数量
    std::vector<BuffData*> mBuffs;          // 数据包缓存池
    IAsk* mSND{ nullptr };                  // 发送编码器
    IAck* mRCV{ nullptr };                  // 接收解码器

public:
    PacketBuffer();
    virtual ~PacketBuffer();

    void    setMaxCacheCount(size_t count);
    size_t  maxCacheCount() const;

public: // 数据包缓存池
    /// @brief 取得一个清空后的数据包
    /// @param dataLen 在协议基础长度之外追加的可变数据长度
    /// @param receive true 创建接收缓存，false 创建发送缓存
    /// @return 数据包
    virtual BuffData* obtain(uint16_t dataLen = 0, bool receive = false) = 0;

    /// @brief 回收数据包
    /// @param buf 数据包
    void recycle(BuffData* buf);

public: // 数据接收场景
    int   add(BuffData* buf, const uint8_t* inBuf, int len);
    bool  complete(BuffData* buf);
    bool  compare(BuffData* src, BuffData* dst);
    bool  check(BuffData* buf);
    IAck* ack(BuffData* bf);

public: // 数据发送场景
    void checkCode(BuffData* buf);

public: // 数据包调试
    /** @brief 将有效数据转换为空格分隔的十六进制字符串。 */
    std::string str(BuffData* buf);
};

/**
 * @brief 绑定具体协议类型、发送包类型和接收包类型的缓存实现。
 * @tparam T 写入 BuffData::type 的协议标识。
 * @tparam Ask IAsk 的具体发送编码类型，需提供 BASE_LEN。
 * @tparam Ack IAck 的具体接收解析类型，需提供 BUFFER_CAPACITY。
 */
template <int16_t T, typename Ask, typename Ack>
class PacketBufferT : public PacketBuffer {
public:
    PacketBufferT() {
        mSND = new Ask();
        mRCV = new Ack();
    }

    /// @brief 取得一个清空后的数据包
    /// @param dataLen 在协议基础长度之外追加的可变数据长度
    /// @param receive true 创建接收缓存，false 创建发送缓存
    /// @return 数据包
    /// @note 优先复用类型和容量匹配的缓存，否则分配新缓存
    BuffData* obtain(uint16_t dataLen = 0, bool receive = false) override {
        const uint16_t baseCapacity = receive ? Ack::BUFFER_CAPACITY : Ask::BASE_LEN;
        if (baseCapacity == 0 || dataLen > UINT16_MAX - baseCapacity) {
            LOGE("Packet buffer length out of range. receive=%d base=%u data=%u max=%u",
                receive, baseCapacity, dataLen, UINT16_MAX);
            return nullptr;
        }

        const uint16_t len = baseCapacity + dataLen;
        for (size_t i = 0; i < mBuffs.size(); ++i) {
            BuffData* bf = mBuffs[i];
            if (bf->type == T && bf->slen == len) {
                bf->len = 0;
                memset(bf->buf, 0, bf->slen);
                mBuffs[i] = mBuffs.back();
                mBuffs.pop_back();
                return bf;
            }
        }

        BuffData* bf = static_cast<BuffData*>(
            calloc(1, sizeof(BuffData) + len));
        if (bf == nullptr) {
            LOGE("Packet buffer allocation failed. len=%d", len);
            return nullptr;
        }
        bf->type = T;
        bf->slen = len;
        bf->len = 0;
        return bf;
    }
};

#endif // !__PACKET_BUFFER_H__
