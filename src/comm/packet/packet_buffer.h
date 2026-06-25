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

#include <list>
#include <stdlib.h>
#include <string>

class IPacketBuffer {
protected:
    std::list<BuffData*> mBuffs;
    IAsk* mSND{ nullptr };
    IAck* mRCV{ nullptr };

public:
    virtual ~IPacketBuffer();

    virtual BuffData* obtain(bool receive = true, uint16_t dataLen = 0) = 0;
    virtual void recycle(BuffData* buf);
    virtual int add(BuffData* buf, uint8_t* inBuf, int len);
    virtual bool complete(BuffData* buf);
    virtual bool compare(BuffData* src, BuffData* dst);
    virtual bool check(BuffData* buf);
    virtual std::string str(BuffData* buf);
    virtual void checkCode(BuffData* buf);
    virtual IAck* ack(BuffData* bf);
};

template <int T, typename Ask, typename Ack>
class IPacketBufferT : public IPacketBuffer {
public:
    IPacketBufferT() {
        mSND = new Ask();
        mRCV = new Ack();
    }

    BuffData* obtain(bool receive = true, uint16_t dataLen = 0) override {
        const uint16_t len = static_cast<uint16_t>((receive ? Ack::BUF_LEN : Ask::MIN_LEN) + dataLen);
        for (auto it = mBuffs.begin(); it != mBuffs.end(); ++it) {
            BuffData* bf = *it;
            if (bf->type == T && bf->slen == len) {
                bf->len = 0;
                memset(bf->buf, 0, bf->slen);
                mBuffs.erase(it);
                return bf;
            }
        }

        BuffData* bf = static_cast<BuffData*>(calloc(1, sizeof(BuffData) + len));
        bf->type = T;
        bf->slen = len;
        bf->len = 0;
        memset(bf->buf, 0, bf->slen);
        return bf;
    }
};

#endif // !__PACKET_BUFFER_H__
