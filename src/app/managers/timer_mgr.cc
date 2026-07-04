/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:49
 * @LastEditTime: 2026-07-04 21:30:00
 * @FilePath: /kk_frame/src/app/managers/timer_mgr.cc
 * @Description: 有限生命周期定时任务管理器
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "timer_mgr.h"

#include <algorithm>
#include <utility>

#include <cdlog.h>
#include <core/systemclock.h>

namespace {
constexpr int64_t kTimerWarnCostMs = 100;

int64_t getNowMs() {
    return cdroid::SystemClock::uptimeMillis();
}
}

TimerMgr::TimerHandle::TimerHandle() { }

TimerMgr::TimerHandle::TimerHandle(TimerMgr* owner, TimerId id)
    : mOwner(owner), mId(id) { }

TimerMgr::TimerHandle::~TimerHandle() {
    cancel();
}

TimerMgr::TimerHandle::TimerHandle(TimerHandle&& other)
    : mOwner(other.mOwner), mId(other.mId) {
    other.mOwner = nullptr;
    other.mId = 0;
}

TimerMgr::TimerHandle& TimerMgr::TimerHandle::operator=(TimerHandle&& other) {
    if (this == &other) return *this;
    cancel();
    mOwner = other.mOwner;
    mId = other.mId;
    other.mOwner = nullptr;
    other.mId = 0;
    return *this;
}

void TimerMgr::TimerHandle::cancel() {
    if (mOwner && mId != 0) {
        mOwner->cancel(mId);
    }
    mOwner = nullptr;
    mId = 0;
}

bool TimerMgr::TimerHandle::isActive() const {
    return mOwner && mId != 0 && mOwner->contains(mId);
}

TimerMgr::TimerMgr() { }

TimerMgr::~TimerMgr() {
    if (mLooper) {
        mLooper->removeEventHandler(this);
    }
    cancelAll();
    for (TimerRecord* record : mRecordPool) {
        delete record;
    }
    mRecordPool.clear();
}

void TimerMgr::init() {
    if (mInited) return;
    mLooper = cdroid::Looper::getForThread();
    if (!mLooper) {
        LOGE("TimerMgr init failed: current thread has no Looper");
        return;
    }
    mLooper->addEventHandler(this);
    mInited = true;
}

size_t TimerMgr::size() const {
    return mRecords.size();
}

void TimerMgr::cancelAll() {
    for (TimerRecord* record : mScheduled) {
        mRecords.erase(record->id);
        recycleRecord(record);
    }
    mScheduled.clear();
    if (mDispatchingRecord) {
        mDispatchingRecord->cancelled = true;
        mRecords.erase(mDispatchingRecord->id);
    }
}

TimerMgr::TimerId TimerMgr::runAfter(int64_t delayMs, Callback callback) {
    return runRepeated(delayMs, 1, std::move(callback), delayMs);
}

TimerMgr::TimerId TimerMgr::runRepeated(int64_t intervalMs,
    uint32_t repeatCount, Callback callback, int64_t firstDelayMs) {
    if (!mInited) {
        LOGE("runRepeated failed: TimerMgr is not initialized");
        return 0;
    }
    if (intervalMs <= 0 || repeatCount == 0 || !callback) {
        LOGE("runRepeated failed: interval=%lld repeat=%u callback=%d",
            (long long)intervalMs, repeatCount, callback ? 1 : 0);
        return 0;
    }
    if (firstDelayMs < 0) firstDelayMs = intervalMs;

    TimerRecord* record = acquireRecord();
    record->id = allocateId();
    if (record->id == 0) {
        LOGE("runRepeated failed: no timer id available");
        recycleRecord(record);
        return 0;
    }
    record->callback = std::move(callback);
    record->intervalMs = intervalMs;
    record->nextFireMs = getNowMs() + firstDelayMs;
    record->repeatCount = repeatCount;

    mRecords[record->id] = record;
    insertOrdered(record);
    return record->id;
}

TimerMgr::TimerHandle TimerMgr::scopedAfter(int64_t delayMs, Callback callback) {
    return TimerHandle(this, runAfter(delayMs, std::move(callback)));
}

TimerMgr::TimerHandle TimerMgr::scopedRepeated(int64_t intervalMs,
    uint32_t repeatCount, Callback callback, int64_t firstDelayMs) {
    return TimerHandle(this, runRepeated(intervalMs, repeatCount,
        std::move(callback), firstDelayMs));
}

bool TimerMgr::cancel(TimerId id) {
    auto found = mRecords.find(id);
    if (found == mRecords.end()) return false;

    TimerRecord* record = found->second;
    mRecords.erase(found);
    if (record == mDispatchingRecord) {
        record->cancelled = true;
        return true;
    }

    auto scheduled = std::find(mScheduled.begin(), mScheduled.end(), record);
    if (scheduled != mScheduled.end()) {
        mScheduled.erase(scheduled);
        recycleRecord(record);
        return true;
    }

    LOGE("cancel failed: timer id=%u has invalid state", id);
    return false;
}

bool TimerMgr::contains(TimerId id) const {
    return id != 0 && mRecords.find(id) != mRecords.end();
}

int TimerMgr::checkEvents() {
    if (!mInited || mScheduled.empty()) return 0;
    return mScheduled.front()->nextFireMs <= getNowMs() ? 1 : 0;
}

int TimerMgr::handleEvents() {
    int firedCount = 0;
    while (true) {
        const int64_t nowMs = getNowMs();
        TimerRecord* record = extractFrontIfDue(nowMs);
        if (!record) break;
        if (!dispatchRecord(record, nowMs)) break;
        ++firedCount;
    }
    return firedCount;
}

TimerMgr::TimerId TimerMgr::allocateId() {
    const TimerId start = mNextId;
    do {
        ++mNextId;
        if (mNextId == 0) ++mNextId;
        if (mRecords.find(mNextId) == mRecords.end()) return mNextId;
    } while (mNextId != start);
    return 0;
}

TimerMgr::TimerRecord* TimerMgr::acquireRecord() {
    TimerRecord* record = nullptr;
    if (mRecordPool.empty()) {
        record = new TimerRecord();
    } else {
        record = mRecordPool.back();
        mRecordPool.pop_back();
    }
    *record = TimerRecord();
    return record;
}

void TimerMgr::recycleRecord(TimerRecord* record) {
    if (!record) return;
    *record = TimerRecord();
    mRecordPool.push_back(record);
}

void TimerMgr::insertOrdered(TimerRecord* record) {
    auto position = mScheduled.begin();
    for (; position != mScheduled.end(); ++position) {
        if (record->nextFireMs < (*position)->nextFireMs ||
            (record->nextFireMs == (*position)->nextFireMs && record->id < (*position)->id)) {
            break;
        }
    }
    mScheduled.insert(position, record);
}

TimerMgr::TimerRecord* TimerMgr::extractFrontIfDue(int64_t nowMs) {
    if (mScheduled.empty() || mScheduled.front()->nextFireMs > nowMs) {
        return nullptr;
    }
    TimerRecord* record = mScheduled.front();
    mScheduled.erase(mScheduled.begin());
    return record;
}

bool TimerMgr::dispatchRecord(TimerRecord* record, int64_t nowMs) {
    if (!record || mDispatchingRecord) {
        LOGE("dispatchRecord failed: invalid or reentrant dispatch");
        if (record) insertOrdered(record);
        return false;
    }

    mDispatchingRecord = record;
    ++record->firedCount;
    const int64_t startMs = getNowMs();
    record->callback(record->id, record->firedCount);
    const int64_t costMs = getNowMs() - startMs;
    if (costMs > kTimerWarnCostMs) {
        LOGW("Timer callback cost too long: id=%u cost=%lldms",
            record->id, (long long)costMs);
    }
    record->nextFireMs = nowMs + record->intervalMs;
    finishDispatch();
    return true;
}

void TimerMgr::finishDispatch() {
    TimerRecord* record = mDispatchingRecord;
    mDispatchingRecord = nullptr;
    if (!record) return;

    if (record->cancelled || record->firedCount >= record->repeatCount) {
        mRecords.erase(record->id);
        recycleRecord(record);
        return;
    }
    insertOrdered(record);
}
