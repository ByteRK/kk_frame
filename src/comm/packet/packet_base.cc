/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-02 00:40:27
 * @LastEditTime: 2026-07-03 00:20:04
 * @FilePath: /kk_frame/src/comm/packet/packet_base.cc
 * @Description: 通讯数据包基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_base.h"

#include <cdlog.h>

IAsk::~IAsk() { }

/// @brief 数据源绑定
/// @param buf 数据源
void IAsk::parse(BuffData* buf) {
    mBuf = buf;
    if (mBuf->len == 0) {
        mBuf->len = mBuf->slen;
    }
}

/// @brief 数据设置
/// @param pos 位置
/// @param data 值
void IAsk::setData(size_t pos, uint8_t data) {
    if (mBuf == nullptr) {
        LOGE("IAsk setData failed: packet buffer is null");
        return;
    }
    if (mBuf->slen <= 0) {
        LOGE("IAsk setData failed: invalid capacity=%d", mBuf->slen);
        return;
    }
    if (pos >= static_cast<size_t>(mBuf->slen)) {
        LOGE("IAsk setData out of bounds. pos=%zu capacity=%d", pos, mBuf->slen);
        return;
    }
    mBuf->buf[pos] = data;
}

/// @brief 数据拷贝
/// @param data 数据指针
/// @param pos 目标起始位置
/// @param len 数据长度
/// @note 范围越界时不写入
void IAsk::setData(const uint8_t* data, size_t pos, size_t len) {
    if (len == 0) {
        return;
    }
    if (mBuf == nullptr) {
        LOGE("IAsk setData failed: packet buffer is null");
        return;
    }
    if (mBuf->slen <= 0) {
        LOGE("IAsk setData failed: invalid capacity=%d", mBuf->slen);
        return;
    }
    if (data == nullptr) {
        LOGE("IAsk setData failed: source data is null. pos=%zu len=%zu", pos, len);
        return;
    }

    const size_t capacity = static_cast<size_t>(mBuf->slen);
    if (pos > capacity || len > capacity - pos) {
        LOGE("IAsk setData out of bounds. pos=%zu len=%zu capacity=%zu",
            pos, len, capacity);
        return;
    }
    memcpy(mBuf->buf + pos, data, len);
}
