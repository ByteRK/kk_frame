/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_buffer.cc
 * @Description: Application packet buffer
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_buffer.h"

#include <iomanip>
#include <sstream>
#include <stdlib.h>

IPacketBuffer::~IPacketBuffer() {
    for (BuffData* bf : mBuffs) {
        free(bf);
    }
    mBuffs.clear();

    delete mSND;
    mSND = nullptr;

    delete mRCV;
    mRCV = nullptr;
}

void IPacketBuffer::recycle(BuffData* buf) {
    if (buf == nullptr) {
        return;
    }

    memset(buf->buf, 0, buf->slen);
    buf->len = 0;
    mBuffs.push_back(buf);
}

int IPacketBuffer::add(BuffData* buf, uint8_t* inBuf, int len) {
    if (buf == nullptr || inBuf == nullptr || len <= 0 || mRCV == nullptr) {
        return 0;
    }

    mRCV->parse(buf);
    return mRCV->add(inBuf, len);
}

bool IPacketBuffer::complete(BuffData* buf) {
    if (buf == nullptr || mRCV == nullptr) {
        return false;
    }

    mRCV->parse(buf);
    return mRCV->complete();
}

bool IPacketBuffer::compare(BuffData* src, BuffData* dst) {
    if (src == nullptr || dst == nullptr) {
        return false;
    }

    if (src->type != dst->type || src->len != dst->len) {
        return false;
    }

    return memcmp(src->buf, dst->buf, src->len) == 0;
}

bool IPacketBuffer::check(BuffData* buf) {
    if (buf == nullptr || mRCV == nullptr) {
        return false;
    }

    mRCV->parse(buf);
    return mRCV->check();
}

std::string IPacketBuffer::str(BuffData* buf) {
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

void IPacketBuffer::checkCode(BuffData* buf) {
    if (buf == nullptr || mSND == nullptr) {
        return;
    }

    mSND->parse(buf);
    mSND->checkCode();
}

IAck* IPacketBuffer::ack(BuffData* bf) {
    if (bf == nullptr || mRCV == nullptr) {
        return nullptr;
    }

    mRCV->parse(bf);
    return mRCV;
}
