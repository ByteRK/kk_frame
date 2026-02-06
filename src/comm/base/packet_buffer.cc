/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:31:51
 * @LastEditTime: 2026-02-06 13:42:02
 * @FilePath: /kk_frame/src/comm/base/packet_buffer.cc
 * @Description: 数据包缓存
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "packet_buffer.h"

#include <iostream>
#include <iomanip>

/////////////////////////////////////// 数据包缓存基类 ///////////////////////////////////////

IPacketBuffer::~IPacketBuffer() {
    for (BuffData* bf : mBuffs) {
        free(bf);
    }
    mBuffs.clear();
    delete mSND;
    delete mRCV;
}

void IPacketBuffer::recycle(BuffData* buf) {
    bzero(buf->buf, buf->slen);
    buf->len = 0;
    mBuffs.push_back(buf);
}

int IPacketBuffer::add(BuffData* buf, uint8_t* in_buf, int len) {
    mRCV->parse(buf);
    return mRCV->add(in_buf, len);
}

bool IPacketBuffer::complete(BuffData* buf) {
    mRCV->parse(buf);
    return mRCV->complete();
}

bool IPacketBuffer::compare(BuffData* src, BuffData* dst) {
    if (src->type != dst->type || src->len != dst->len)
        return false;
    for (short i = 0; i < src->len; i++) {
        if (src->buf[i] != dst->buf[i])
            return false;
    }
    return true;
}

bool IPacketBuffer::check(BuffData* buf) {
    mRCV->parse(buf);
    return mRCV->check();
}

std::string IPacketBuffer::str(BuffData* buf) {
    std::ostringstream oss;
    for (int i = 0; i < buf->len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)buf->buf[i] << " ";
    }
    return oss.str();
}

void IPacketBuffer::checkCode(BuffData* buf) {
    mSND->parse(buf);
    mSND->checkCode();
}

IAck* IPacketBuffer::ack(BuffData* bf) {
    mRCV->parse(bf);
    return mRCV;
}
