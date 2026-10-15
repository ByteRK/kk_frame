/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-15 00:00:00
 * @LastEditTime: 2026-10-15 10:45:59
 * @FilePath: /kk_frame/src/comm/packet/ask_ack.cc
 * @Description: 等待确认重发机制（stop-and-wait）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "ask_ack.h"

#include <cdlog.h>
#include <core/systemclock.h>

#include <string.h>

AskMustAck::AskMustAck(PacketBufferPool* pool, SendFunc sender)
    : mPool(pool), mSender(sender) {
}

AskMustAck::~AskMustAck() {
    stop();
    for (const Packet& p : mPackets) {
        if (mPool) mPool->recycle(p.bd);
    }
    mPackets.clear();
}

void AskMustAck::start(int64_t tickMs) {
    if (mStarted) return;
    mStarted = true;
    setTick(tickMs);
    startTick(tickMs);
}

void AskMustAck::stop() {
    if (!mStarted) return;
    mStarted = false;
    stopTick();
}

int AskMustAck::add(BuffData** ppbd, int ackCmd) {
    BuffData* bd = ppbd ? *ppbd : nullptr;
    if (bd == nullptr || bd->len == 0) {
        LOGE("AskMustAck add failed: invalid buff. len=%d", bd ? (int)bd->len : -1);
        return -1;
    }
    if (mPool == nullptr) {
        LOGE("AskMustAck add failed: pool is null");
        return -1;
    }

    const int64_t now = cdroid::SystemClock::uptimeMillis();

    // 去重：同 ackCmd 已存在则更新数据（并移到队尾）
    for (size_t i = 0; i < mPackets.size(); ++i) {
        if (mPackets[i].ackCmd != ackCmd) continue;

        Packet& p = mPackets[i];
        if (bd->len > p.bd->slen) {
            LOGE("AskMustAck add failed: data exceeds capacity. len=%d cap=%d",
                (int)bd->len, (int)p.bd->slen);
            mPool->recycle(bd);
            *ppbd = nullptr;
            return -1;
        }

        memcpy(p.bd->buf, bd->buf, bd->len);
        p.bd->len = bd->len;
        p.addTime = now;

        if (i < mPackets.size() - 1) {
            LOGW("AskMustAck packet exists, move tail. ackCmd=%d pos=%zu", ackCmd, i);
            Packet moved = p;
            mPackets.erase(mPackets.begin() + i);
            mPackets.push_back(moved);
        } else {
            LOGW("AskMustAck packet exists last, update data. ackCmd=%d pos=%zu", ackCmd, i);
        }

        mPool->recycle(bd);
        *ppbd = nullptr;
        return 0;
    }

    // 新包入队
    Packet p;
    p.ackCmd  = ackCmd;
    p.addTime = now;
    p.bd      = bd;
    mPackets.push_back(p);
    *ppbd = nullptr;
    LOGD("AskMustAck new packet, add tail. ackCmd=%d size=%zu", ackCmd, mPackets.size());
    return 0;
}

int AskMustAck::add(IAsk* ask) {
    if (ask == nullptr) {
        LOGE("AskMustAck add failed: ask is null");
        return -1;
    }
    BuffData* bd = ask->buffer();
    return add(&bd, ask->getCMD());
}

int AskMustAck::del(const IAck* ack) {
    if (mPackets.empty()) {
        LOGW("AskMustAck del failed: empty queue");
        return -1;
    }
    if (ack == nullptr) {
        LOGE("AskMustAck del failed: ack is null");
        return -1;
    }
    if (mPool == nullptr) {
        LOGE("AskMustAck del failed: pool is null");
        return -1;
    }

    Packet& p = mPackets.front();
    if (p.ackCmd != ack->getCMD()) {
        LOGE("AskMustAck del mismatch. ack=%d expect=%d", ack->getCMD(), p.ackCmd);
        return -1;
    }

    mPool->recycle(p.bd);
    mPackets.erase(mPackets.begin());
    LOGD("AskMustAck del ok. ack=%d size=%zu", ack->getCMD(), mPackets.size());
    return 0;
}

BuffData* AskMustAck::get() {
    if (mPackets.empty() || mPool == nullptr) return nullptr;

    const Packet& p = mPackets.front();
    BuffData* bd = mPool->obtainLike(p.bd);
    if (bd == nullptr) {
        LOGE("AskMustAck get failed: obtainLike failed");
        return nullptr;
    }
    bd->len = p.bd->len;
    memcpy(bd->buf, p.bd->buf, p.bd->len);
    return bd;
}

void AskMustAck::onTick(int64_t nowMs) {
    if (mPackets.empty()) return;

    Packet& p = mPackets.front();
    if (nowMs - p.addTime < mTimeoutMs) return;

    mTimeout++;
    LOGW("AskMustAck timeout. times=%d ackCmd=%d", mTimeout, p.ackCmd);

    // 记录队首，防止重发过程中（如回调内触发 ack）被移除导致引用失效
    BuffData* frontBd = p.bd;

    // 超时重发队首包
    BuffData* bd = get();
    if (bd != nullptr && mSender) {
        mSender(bd);
    }

    // 若队首仍在（未被 ack 移除），重置计时等待下一轮超时
    if (!mPackets.empty() && mPackets.front().bd == frontBd) {
        mPackets.front().addTime = nowMs;
    }
}
