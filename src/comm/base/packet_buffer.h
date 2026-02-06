/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:31:51
 * @LastEditTime: 2026-02-06 13:41:43
 * @FilePath: /kk_frame/src/comm/base/packet_buffer.h
 * @Description: 数据包缓存
 * @BugList: 
 * 
 * Copyright (c) 2025 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __PACKET_BUFFER_H__
#define __PACKET_BUFFER_H__

#include "packet_base.h"
#include "mcu_ui.h"
#include <list>

/// @brief 数据包缓存基类
class IPacketBuffer {
protected:
    std::list<BuffData *> mBuffs;
    IAsk                 *mSND = nullptr;
    IAck                 *mRCV = nullptr;
public:
    virtual ~IPacketBuffer();
public:
    virtual BuffData   *obtain(bool receive = true, uint16_t dataLen = 0)  = 0;  // 创建(receive用与区分发送还是接收，dataLen用于不定长包)
    virtual void        recycle(BuffData *buf);                                  // 回收
    virtual int         add(BuffData *buf, uint8_t *in_buf, int len);            // 添加数据
    virtual bool        complete(BuffData *buf);                                 // 数据完整
    virtual bool        compare(BuffData *src, BuffData *dst);                   // 对比数据
    virtual bool        check(BuffData *buf);                                    // 校验数据
    virtual std::string str(BuffData *buf);                                      // 格式化字符串
    virtual void        checkCode(BuffData *buf);                                // 生成校验码
    virtual IAck       *ack(BuffData *bf);                                       // 转化成ack
};

/// @brief 数据包缓存模板类
/// @tparam T 数据包类型
/// @tparam Ask 发送数据包类
/// @tparam Ack 接收数据包类
template <BufferType T, typename Ask, typename Ack>
class IPacketBufferT : public IPacketBuffer {
public:
    IPacketBufferT() {
        mSND = new Ask();
        mRCV = new Ack();
    }
    BuffData* obtain(bool receive, uint16_t dataLen) override {
        uint8_t len = (receive ? Ack::BUF_LEN : Ask::MIN_LEN) + dataLen;
        for (auto it = mBuffs.begin(); it != mBuffs.end(); it++) {
            BuffData* bf = *it;
            if (bf->type == T && bf->slen == len) {
                bf->len = 0;
                mBuffs.erase(it);
                return bf;
            }
        }
        BuffData* bf = (BuffData*)calloc(1, sizeof(BuffData) + len);
        bf->type = T;
        bf->slen = len;
        bf->len = 0;
        bzero(bf->buf, bf->slen);
        return bf;
    }
};

#endif // !__PACKET_BUFFER_H__
