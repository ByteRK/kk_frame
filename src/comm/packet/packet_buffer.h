/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_buffer.h
 * @Description: Application packet buffer
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_BUFFER_H__
#define __PACKET_BUFFER_H__

#include "packet_base.h"

#include <cdlog.h>

#include <limits>
#include <stdlib.h>
#include <string>
#include <vector>

/**
 * @brief 数据包缓存池及协议编解码器的统一入口。
 *
 * obtain() 取得的 BuffData 由调用者暂时持有，使用完必须交回 recycle()。
 * 类内部持有并负责销毁发送解析器 mSND、接收解析器 mRCV 和已回收缓存。
 */
class IPacketBuffer {
public:
    static constexpr size_t DEFAULT_MAX_CACHE_COUNT = 16;

protected:
    std::vector<BuffData*> mBuffs; // 已回收缓存池
    size_t mMaxCacheCount{ DEFAULT_MAX_CACHE_COUNT };
    IAsk* mSND{ nullptr };
    IAck* mRCV{ nullptr };

public:
    /** @brief 创建缓存池并按默认最大缓存数量预留指针空间。 */
    IPacketBuffer();
    virtual ~IPacketBuffer();

    /**
     * @brief 设置最多保留的空闲缓存数量，超出部分会告警并释放。
     * @param count 最大缓存数量；设置为 0 表示不保留空闲缓存。
     */
    void setMaxCacheCount(size_t count);
    /** @brief 返回当前设置的最大空闲缓存数量。 */
    size_t maxCacheCount() const;

    /**
     * @brief 取得一个清空后的数据包缓存。
     * @param dataLen 在协议基础长度之外追加的可变数据长度。
     * @param receive true 创建接收缓存，false 创建发送缓存。
     */
    virtual BuffData* obtain(size_t dataLen = 0, bool receive = false) = 0;
    /** @brief 清空缓存并放回池中；传入 nullptr 时无操作。 */
    virtual void recycle(BuffData* buf);
    /** @brief 将输入字节追加到接收缓存，返回实际消费的输入长度。 */
    virtual int add(BuffData* buf, const uint8_t* inBuf, int len);
    /** @brief 判断接收缓存是否已经组成完整数据包。 */
    virtual bool complete(BuffData* buf);
    /** @brief 比较两个缓存的协议类型、有效长度和数据内容是否完全一致。 */
    virtual bool compare(BuffData* src, BuffData* dst);
    /** @brief 使用接收解析器校验完整数据包。 */
    virtual bool check(BuffData* buf);
    /** @brief 将有效数据转换为空格分隔的十六进制字符串。 */
    virtual std::string str(BuffData* buf);
    /** @brief 使用发送解析器计算并写入校验值。 */
    virtual void checkCode(BuffData* buf);
    /** @brief 将接收解析器绑定到 bf 并返回；返回对象由本类持有。 */
    virtual IAck* ack(BuffData* bf);
};

/**
 * @brief 绑定具体协议类型、发送包类型和接收包类型的缓存实现。
 * @tparam T 写入 BuffData::type 的协议标识。
 * @tparam Ask IAsk 的具体发送编码类型。
 * @tparam Ack IAck 的具体接收解析类型。
 */
template <int T, typename Ask, typename Ack>
class IPacketBufferT : public IPacketBuffer {
public:
    IPacketBufferT() {
        mSND = new Ask();
        mRCV = new Ack();
    }

    /** @brief 优先复用类型和容量匹配的缓存，否则分配新缓存。 */
    BuffData* obtain(size_t dataLen = 0, bool receive = false) override {
        const size_t baseLen = receive ? static_cast<size_t>(Ack::BUF_LEN)
            : static_cast<size_t>(Ask::MIN_LEN);
        const size_t maxLen = static_cast<size_t>(std::numeric_limits<short>::max());
        if (baseLen == 0 || baseLen > maxLen || dataLen > maxLen - baseLen) {
            LOGE("Packet buffer length out of range. receive=%d base=%zu data=%zu max=%zu",
                receive, baseLen, dataLen, maxLen);
            return nullptr;
        }

        const short len = static_cast<short>(baseLen + dataLen);
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
            calloc(1, sizeof(BuffData) + static_cast<size_t>(len)));
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
