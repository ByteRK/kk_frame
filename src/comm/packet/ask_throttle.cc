/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-30 15:00:00
 * @LastEditTime: 2026-10-30 15:00:00
 * @FilePath: /kk_frame/src/comm/packet/ask_throttle.cc
 * @Description: 发送限流器实现
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "ask_throttle.h"

#include <cdlog.h>

#include <string.h>

constexpr size_t  AskThrottle::DEFAULT_MAX_QUEUE;
constexpr int64_t AskThrottle::DEFAULT_INTERVAL_MS;

AskThrottle::AskThrottle(PacketBufferPool* pool, SendFunc sender)
    : mPool(pool), mSender(sender) {
}

AskThrottle::~AskThrottle() {
    stop();
    clear();
}

void AskThrottle::start(int64_t intervalMs) {
    setInterval(intervalMs);
    if (mStarted) return;
    mStarted = true;
    mHasSent = false;
    mLastSendMs = 0;
    const int64_t tickMs = tickOf(mIntervalMs);
    setTick(tickMs);
    startTick(tickMs);
}

void AskThrottle::stop() {
    if (!mStarted) return;
    mStarted = false;
    stopTick();
}

void AskThrottle::setInterval(int64_t ms) {
    if (ms <= 0) {
        LOGE("AskThrottle setInterval failed: invalid interval. ms=%lld", (long long)ms);
        return;
    }
    if (mIntervalMs == ms) return;
    mIntervalMs = ms;
    if (mStarted) setTick(tickOf(mIntervalMs));
}

void AskThrottle::setMaxQueue(size_t count) {
    if (count == 0) {
        LOGE("AskThrottle setMaxQueue failed: invalid count");
        return;
    }
    mMaxQueue = count;
    if (mPackets.size() > mMaxQueue) {
        LOGW("AskThrottle queue exceeds new limit. size=%zu max=%zu",
            mPackets.size(), mMaxQueue);
    }
}

int AskThrottle::add(BuffData** ppbd, int cmd) {
    BuffData* bd = ppbd ? *ppbd : nullptr;
    if (bd == nullptr || bd->len == 0) {
        LOGE("AskThrottle add failed: invalid buff. len=%d", bd ? (int)bd->len : -1);
        return -1;
    }
    if (mPool == nullptr) {
        LOGE("AskThrottle add failed: pool is null");
        return -1;
    }

    // 同命令合并：仅更新队列中的最新数据，保持原有排队位置（未发出的都应是最新状态）
    if (mCoalesceByCmd) {
        for (size_t i = 0; i < mPackets.size(); ++i) {
            if (mPackets[i].cmd != cmd) continue;

            Packet& p = mPackets[i];
            if (bd->len > p.bd->slen) {
                LOGE("AskThrottle add failed: data exceeds capacity. len=%d cap=%d",
                    (int)bd->len, (int)p.bd->slen);
                mPool->recycle(bd);
                *ppbd = nullptr;
                return -1;
            }

            memcpy(p.bd->buf, bd->buf, bd->len);
            p.bd->len = bd->len;

            mPool->recycle(bd);
            *ppbd = nullptr;
            LOGD("AskThrottle packet coalesced. cmd=%d pos=%zu size=%zu",
                cmd, i, mPackets.size());
            return 0;
        }
    }

    // 队列满载：丢新包，交由调用方决定是否重试
    if (mPackets.size() >= mMaxQueue) {
        mDropCount++;
        LOGE("AskThrottle add failed: queue full. size=%zu max=%zu cmd=%d drops=%lld",
            mPackets.size(), mMaxQueue, cmd, (long long)mDropCount);
        mPool->recycle(bd);
        *ppbd = nullptr;
        return -1;
    }

    Packet p;
    p.cmd = cmd;
    p.bd  = bd;
    mPackets.push_back(p);
    *ppbd = nullptr;
    LOGD("AskThrottle new packet, add tail. cmd=%d size=%zu", cmd, mPackets.size());
    return 0;
}

int AskThrottle::add(IAsk* ask) {
    if (ask == nullptr) {
        LOGE("AskThrottle add failed: ask is null");
        return -1;
    }
    BuffData* bd = ask->buffer();
    return add(&bd, ask->getCMD());
}

void AskThrottle::clear() {
    if (mPackets.empty()) return;

    const size_t count = mPackets.size();
    for (const Packet& p : mPackets) {
        if (mPool) mPool->recycle(p.bd);
    }
    mPackets.clear();
    LOGD("AskThrottle clear. drops=%zu", count);
}

void AskThrottle::onTick(int64_t nowMs) {
    if (mPackets.empty()) return;

    if (mSender == nullptr) {
        LOGW("AskThrottle sender is null, drop queued packets. size=%zu",
            mPackets.size());
        clear();
        return;
    }

    // 未到间隔直接返回；注意每次发送后重置计时，不做"补发"追赶
    if (mHasSent && nowMs - mLastSendMs < mIntervalMs) return;

    const Packet p = mPackets.front();
    mPackets.pop_front();

    mHasSent = true;
    mLastSendMs = nowMs;
    mSentCount++;

    // 数据包所有权转移给发送回调，由通道负责回收
    const int rc = mSender(p.bd);
    if (rc != 0) {
        LOGW("AskThrottle send failed. rc=%d cmd=%d size=%zu",
            rc, p.cmd, mPackets.size());
    } else {
        LOGD("AskThrottle send ok. cmd=%d size=%zu", p.cmd, mPackets.size());
    }
}

int64_t AskThrottle::tickOf(int64_t intervalMs) {
    int64_t tickMs = intervalMs / 4;
    if (tickMs < 10) tickMs = 10;
    if (tickMs > 50) tickMs = 50;
    return tickMs;
}
