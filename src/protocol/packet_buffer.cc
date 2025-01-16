
#include <iostream>
#include <iomanip>
#include "packet_buffer.h"

SDHWPacketBuffer::SDHWPacketBuffer() {
}

SDHWPacketBuffer::~SDHWPacketBuffer() {
    for (BuffData* bf : mBuffs) {
        free(bf);
    }
    mBuffs.clear();
}

void SDHWPacketBuffer::setType(BufferType type, BufferType cmd) {
    mSND.setType(type);
    mRCV.setType(type);
    mRCV.setCMD(cmd);
}

BuffData* SDHWPacketBuffer::obtain(BufferType type) {
    short len = 0xFF;
    switch (type) {
    case BT_MCU:
        len = MCU2UI::BUF_LEN; break;
    case BT_BTN:
        len = MCU2UI::BUF_LEN; break;
    case BT_TUYA:
        len = MCU2UI::BUF_LEN; break;
    default:
        LOGE("当前未指定数据包长度,已使用默认值,建议进行设置");break;
    }

    for (auto it = mBuffs.begin(); it != mBuffs.end(); it++) {
        BuffData* bf = *it;
        if (bf->type == type && bf->slen == len) {
            bf->len = 0;
            mBuffs.erase(it);
            return bf;
        }
    }

    BuffData* bf = (BuffData*)calloc(1, sizeof(BuffData) + len);
    bf->type = type;
    bf->slen = len;
    bf->len = 0;
    bzero(bf->buf, bf->slen);

    return bf;
}

BuffData* SDHWPacketBuffer::obtain(BufferType type, uint16_t datalen) {
    short len = datalen;
    switch (type) {
    case BT_MCU:
        if (datalen == 0xFFFF)
            len = 7;
        else
            len = MCU_LEN_SEND + datalen;
        break;
    case BT_BTN:
        len = FILM_LEN_SEND + datalen; break;
    case BT_TUYA:
        len = TUYA_LEN_SEND + datalen; break;
    default:
        LOGE("当前未指定数据包长度,已直接使用datalen,建议进行设置");break;
    }

    for (auto it = mBuffs.begin(); it != mBuffs.end(); it++) {
        BuffData* bf = *it;
        if (bf->type == type && bf->slen == len) {
            bf->len = 0;
            mBuffs.erase(it);
            return bf;
        }
    }

    BuffData* bf = (BuffData*)calloc(1, sizeof(BuffData) + len);
    bf->type = type;
    bf->slen = len;
    bf->len = 0;
    bzero(bf->buf, bf->slen);

    return bf;
}

void SDHWPacketBuffer::recycle(BuffData* buf) {
    bzero(buf->buf, buf->slen);
    buf->len = 0;
    mBuffs.push_back(buf);
}

int SDHWPacketBuffer::add(BuffData* buf, uint8_t* in_buf, int len) {
    mRCV.parse(buf);
    return mRCV.add(in_buf, len);
}

bool SDHWPacketBuffer::complete(BuffData* buf) {
    mRCV.parse(buf);
    return mRCV.complete();
}

bool SDHWPacketBuffer::compare(BuffData* src, BuffData* dst) {
    // 有按键数据每一帧都需要处理
    if (src->type != dst->type || src->len != dst->len)
        return false;
    for (short i = 0; i < src->len; i++) {
        if (src->buf[i] != dst->buf[i])
            return false;
    }
    return true;
}

bool SDHWPacketBuffer::check(BuffData* buf) {
    mRCV.parse(buf);
    return mRCV.check();
}

std::string SDHWPacketBuffer::str(BuffData* buf) {
    std::ostringstream oss;
    for (int i = 0; i < buf->len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)buf->buf[i] << " ";
    }
    return oss.str();
}

void SDHWPacketBuffer::check_code(BuffData* buf) {
    mSND.parse(buf);
    mSND.checkcode();
}

IAck* SDHWPacketBuffer::ack(BuffData* bf) {
    mRCV.parse(bf);
    return &mRCV;
}
