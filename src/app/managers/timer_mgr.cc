/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:49
 * @LastEditTime: 2026-07-05 00:29:10
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

    /// @brief 获取当前时间戳
    /// @return 毫秒时间戳
    int64_t getNowMs() {
        return cdroid::SystemClock::uptimeMillis();
    }
}

/// @brief 自动挡任务句柄 [空白构造]
TimerMgr::TimerHandle::TimerHandle() { }

/// @brief 自动挡任务句柄析构
/// @note 如果任务还在调度中，会自动取消
TimerMgr::TimerHandle::~TimerHandle() {
    cancel();
}

/// @brief 自动挡任务句柄 [TimerMgr 带任务关联]
/// @param owner TimerMgr 对象
/// @param id 任务id
/// @note 该接口仅允许 TimerMgr 内部调用
TimerMgr::TimerHandle::TimerHandle(TimerMgr* owner, TimerId id)
    : mOwner(owner), mId(id) { }

/// @brief 自动挡任务句柄 [移动构造]
/// @param other 源句柄
/// @note 源句柄转为无效状态
TimerMgr::TimerHandle::TimerHandle(TimerHandle&& other)
    : mOwner(other.mOwner), mId(other.mId) {
    other.mOwner = nullptr;
    other.mId = 0;
}

/// @brief 自动挡任务句柄 [移动赋值]
/// @param other 源句柄
/// @return 当前句柄
/// @note 当前任务自动取消再进行接管新任务，源句柄转为无效状态
TimerMgr::TimerHandle& TimerMgr::TimerHandle::operator=(TimerHandle&& other) {
    if (this == &other) return *this;
    cancel();
    mOwner = other.mOwner;
    mId = other.mId;
    other.mOwner = nullptr;
    other.mId = 0;
    return *this;
}

/// @brief 取消任务
void TimerMgr::TimerHandle::cancel() {
    if (mOwner && mId != 0) {
        mOwner->cancel(mId);
    }
    mOwner = nullptr;
    mId = 0;
}

/// @brief 当前任务是否在运行中
/// @return true: 在运行中
bool TimerMgr::TimerHandle::isActive() const {
    return mOwner && mId != 0 && mOwner->contains(mId);
}

/// @brief 获取任务ID
/// @return 任务ID；0 表示未在运行中
TimerMgr::TimerId TimerMgr::TimerHandle::id() const {
    return mId;
}

/// @brief 定时任务管理器构造
TimerMgr::TimerMgr() { }

/// @brief 定时任务管理器析构
/// @note 取消所有任务并释放资源
TimerMgr::~TimerMgr() {
    if (mLooper) mLooper->removeEventHandler(this);
    cancelAll();
    for (TimerRecord* record : mRecordPool)
        delete record;
    mRecordPool.clear();
}

/// @brief 初始化定时任务管理器
void TimerMgr::init() {
    if (mInited) return;
    mLooper = cdroid::Looper::getMainLooper();
    if (!mLooper) {
        LOGE("TimerMgr init failed: current thread has no Looper");
        return;
    }
    mLooper->addEventHandler(this);
    mInited = true;
}

/// @brief 获取当前有效任务数量
/// @return 等待执行和正在回调的任务总数
size_t TimerMgr::size() const {
    return mRecords.size();
}

/// @brief 取消全部任务
/// @note 正在回调的任务会被标记取消，并在回调结束后回收
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

/// @brief 延迟执行任务
/// @param delayMs 延迟时间，单位毫秒，必须大于 0
/// @param callback 回调函数
/// @return 任务ID；0 表示任务创建失败
TimerMgr::TimerId TimerMgr::runAfter(int64_t delayMs, Callback callback) {
    return runRepeated(delayMs, 1, std::move(callback), delayMs);
}

/// @brief 重复执行任务
/// @param intervalMs 任务间隔，单位毫秒，必须大于 0
/// @param repeatCount 重复次数，必须大于 0
/// @param callback 回调函数
/// @param firstDelayMs 首次延迟时间，单位毫秒，必须大于等于 0
/// @return 任务ID；0 表示任务创建失败
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

/// @brief 延迟执行任务 [带自动挡任务句柄]
/// @param delayMs 延迟时间，单位毫秒，必须大于 0
/// @param callback 回调函数
/// @return 任务句柄；无效句柄表示任务创建失败
/// @note 禁止使用临时变量接受句柄，否则任务会自动取消
TimerMgr::TimerHandle TimerMgr::scopedAfter(int64_t delayMs, Callback callback) {
    return TimerHandle(this, runAfter(delayMs, std::move(callback)));
}

/// @brief 重复执行任务 [带自动挡任务句柄]
/// @param intervalMs 任务间隔，单位毫秒，必须大于 0
/// @param repeatCount 重复次数，必须大于 0
/// @param callback 回调函数
/// @param firstDelayMs 首次延迟时间，单位毫秒，必须大于等于 0
/// @return 任务句柄；无效句柄表示任务创建失败
/// @note 禁止使用临时变量接受句柄，否则任务会自动取消
TimerMgr::TimerHandle TimerMgr::scopedRepeated(int64_t intervalMs,
    uint32_t repeatCount, Callback callback, int64_t firstDelayMs) {
    return TimerHandle(this, runRepeated(intervalMs, repeatCount,
        std::move(callback), firstDelayMs));
}

/// @brief 取消指定任务
/// @param id 任务ID
/// @return true: 取消成功；false: 任务不存在或已取消
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

/// @brief 判断指定任务是否存在
/// @param id 任务ID
/// @return true: 存在；false: 不存在
bool TimerMgr::contains(TimerId id) const {
    return id != 0 && mRecords.find(id) != mRecords.end();
}

/// @brief 检查是否有任务需要执行
/// @return 1: 有任务需要执行；0: 没有任务需要执行
int TimerMgr::checkEvents() {
    if (!mInited || mScheduled.empty()) return 0;
    return mScheduled.front()->nextFireMs <= getNowMs() ? 1 : 0;
}

/// @brief 依次执行本轮所有已到期任务
/// @return 执行的任务数量
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

/// @brief 分配唯一任务标识
/// @return 任务标识；0 表示分配失败
TimerMgr::TimerId TimerMgr::allocateId() {
    const TimerId start = mNextId;
    do {
        ++mNextId;
        if (mNextId == 0) ++mNextId;
        if (mRecords.find(mNextId) == mRecords.end()) return mNextId;
    } while (mNextId != start);
    return 0;
}

/// @brief 从缓存池获取任务记录实例
/// @return 任务记录实例
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

/// @brief 重置并回收任务记录实例
/// @param record 任务记录实例
void TimerMgr::recycleRecord(TimerRecord* record) {
    if (!record) return;
    *record = TimerRecord();
    mRecordPool.push_back(record);
}

/// @brief 将任务记录插入到有序队列中
/// @param record 任务记录实例
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

/// @brief 从有序队列中提取第一个已到期任务
/// @param nowMs 当前时间，单位毫秒
/// @return 任务记录实例；为空表示没有已到期任务
TimerMgr::TimerRecord* TimerMgr::extractFrontIfDue(int64_t nowMs) {
    if (mScheduled.empty() || mScheduled.front()->nextFireMs > nowMs) {
        return nullptr;
    }
    TimerRecord* record = mScheduled.front();
    mScheduled.erase(mScheduled.begin());
    return record;
}

/// @brief 执行任务
/// @param record 任务记录实例
/// @param nowMs 当前时间，单位毫秒
/// @return true: 执行成功；false: 执行失败
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

/// @brief 任务执行收尾，进行回收或重新调度
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
