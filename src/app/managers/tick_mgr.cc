/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-24 10:27:27
 * @FilePath: /kk_frame/src/app/managers/tick_mgr.cc
 * @Description: Tick 管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "tick_mgr.h"

#include <cdlog.h>
#include <core/systemclock.h>

namespace {
constexpr int64_t kTickWarnCostMs = 100;
}

/// @brief 获取当前时间戳
/// @return 毫秒时间戳
static int64_t getNowMs() {
    return cdroid::SystemClock::uptimeMillis();
}

/// @brief Tick 监听对象析构
TickMgr::ITickClass::~ITickClass() {
#if 0 // 降低性能影响，应用层自行管控
    stopTick();
#endif
}

/// @brief 设置 Tick 间隔
/// @param intervalMs 间隔时间（毫秒）
void TickMgr::ITickClass::setTick(int64_t intervalMs) {
    mTickIntervalMs = intervalMs;
}

/// @brief 开始 Tick 任务
/// @param firstDelayMs 首次调度延迟：-1 立即执行；0 下一轮事件循环执行；>0 延迟指定毫秒后执行
void TickMgr::ITickClass::startTick(int64_t firstDelayMs) {
    if (!g_tick->hasTick(this)) {
        g_tick->addTick(this, mTickIntervalMs, firstDelayMs);
    } else if (firstDelayMs < 0) {
        g_tick->runTickNow(this);
    }
}

/// @brief 停止 Tick 任务
void TickMgr::ITickClass::stopTick() {
    if (g_tick->hasTick(this)) g_tick->removeTick(this);
}

/// @brief Tick 管理器构造
TickMgr::TickMgr() { }

/// @brief Tick 管理器析构
TickMgr::~TickMgr() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
    clear();
    if (mDispatchingRecord) {
        destroyRecord(mDispatchingRecord);
        mDispatchingRecord = nullptr;
    }
    destroyRecordList(mListeners);
    destroyRecordList(mRecordPool);
}

/// @brief 初始化 TickMgr
void TickMgr::init() {
    cdroid::Looper::getMainLooper()->addEventHandler(this);
}

/// @brief 获取已注册任务数量
/// @return 数量
size_t TickMgr::size() const {
    size_t count = mListeners.size();
    if (mDispatchingRecord && !mDispatchingRecord->cancelled) {
        ++count;
    }
    return count;
}

/// @brief 清除所有任务
void TickMgr::clear() {
    for (size_t i = 0; i < mListeners.size(); ++i) {
        recycleRecord(mListeners[i]);
        mListeners[i] = nullptr;
    }
    mListeners.clear();
    if (mDispatchingRecord) {
        mDispatchingRecord->cancelled = true;
    }
}

/// @brief 添加任务
/// @param listener 任务指针
/// @param intervalMs 间隔时间，必须大于 0
/// @param firstDelayMs 首次调度延迟：-1 立即执行；0 下一轮 check/handle 时执行；>0 延迟指定毫秒后执行
/// @return 状态
bool TickMgr::addTick(ITickClass* listener, int64_t intervalMs, int64_t firstDelayMs) {
    if (!listener) {
        LOGE("addTick failed: listener is nullptr");
        return false;
    }
    if (intervalMs <= 0) {
        LOGE("addTick failed: invalid intervalMs=%lld", (long long)intervalMs);
        return false;
    }
    if (isRegistered(listener)) {
        LOGE("addTick failed: listener already exists, this=%p", listener);
        return false;
    }

    const int64_t nowMs = getNowMs();
    TickRecord* record = acquireRecord();
    record->listener = listener;
    record->intervalMs = intervalMs;

    if (firstDelayMs < 0) {
        record->nextFireMs = nowMs;
        if (!isDispatching()) {
            return dispatchRecord(record, nowMs);
        }
        insertOrdered(record);
        return true;
    }

    record->nextFireMs = nowMs + firstDelayMs;
    insertOrdered(record);
    return true;
}

/// @brief 移除任务
/// @param listener 任务指针
/// @return 状态
bool TickMgr::removeTick(ITickClass* listener) {
    if (!listener) {
        LOGE("removeTick failed: listener is nullptr");
        return false;
    }
    TickRecord* record = extractScheduledRecord(listener);
    if (record) {
        recycleRecord(record);
        return true;
    }
    TickRecord* dispatching = findDispatchingRecord(listener);
    if (dispatching) {
        dispatching->cancelled = true;
        return true;
    }
    LOGE("removeTick failed: listener not found, this=%p", listener);
    return false;
}

/// @brief 判断任务是否已注册
/// @param listener 任务指针
/// @return 状态
bool TickMgr::hasTick(const ITickClass* listener) const {
    if (!listener) {
        LOGE("hasTick failed: listener is nullptr");
        return false;
    }
    return isRegistered(listener);
}

/// @brief 立即执行指定任务一次
/// @param listener 任务指针
/// @return 状态
bool TickMgr::runTickNow(ITickClass* listener) {
    if (!listener) {
        LOGE("runTickNow failed: listener is nullptr");
        return false;
    }
    if (isDispatching()) {
        LOGE("runTickNow failed: another record is dispatching, this=%p, current=%p",
            listener, mDispatchingRecord->listener);
        return false;
    }
    TickRecord* record = extractScheduledRecord(listener);
    if (!record) {
        LOGE("runTickNow failed: listener not found in scheduled list, this=%p", listener);
        return false;
    }
    return dispatchRecord(record, getNowMs());
}

/// @brief 更新任务周期
/// @param listener 任务指针
/// @param intervalMs 新周期，必须大于 0
/// @param restartFromNow 是否从当前时刻重新计时
/// @return 状态
bool TickMgr::updateInterval(ITickClass* listener, int64_t intervalMs, bool restartFromNow) {
    if (!listener) {
        LOGE("updateInterval failed: listener is nullptr");
        return false;
    }
    if (intervalMs <= 0) {
        LOGE("updateInterval failed: invalid intervalMs=%lld", (long long)intervalMs);
        return false;
    }

    TickRecord* record = extractScheduledRecord(listener);
    if (record) {
        record->intervalMs = intervalMs;
        if (restartFromNow) {
            record->nextFireMs = getNowMs() + intervalMs;
        }
        insertOrdered(record);
        return true;
    }

    TickRecord* dispatching = findDispatchingRecord(listener);
    if (dispatching) {
        dispatching->intervalMs = intervalMs;
        if (restartFromNow) {
            dispatching->nextFireMs = getNowMs() + intervalMs;
        }
        return true;
    }

    LOGE("updateInterval failed: listener not found, this=%p", listener);
    return false;
}

/// @brief 检查是否存在到期任务
/// @return 非 0 表示存在到期任务
int TickMgr::checkEvents() {
    const TickRecord* front = peekFront();
    return (front && front->nextFireMs <= getNowMs()) ? 1 : 0;
}

/// @brief 执行全部到期任务
/// @return 本轮实际执行的任务数量
int TickMgr::handleEvents() {
    int firedCount = 0;
    while (true) {
        const int64_t nowMs = getNowMs();
        if (isDispatching()) {
            LOGE("handleEvents failed: reentrant dispatch detected, current=%p",
                mDispatchingRecord->listener);
            break;
        }
        TickRecord* record = extractFrontIfDue(nowMs);
        if (!record) {
            break;
        }
        if (!dispatchRecord(record, nowMs)) {
            LOGE("handleEvents failed: dispatchRecord failed");
            break;
        }
        ++firedCount;
    }
    return firedCount;
}

/// @brief 比较两个任务的调度优先级
/// @param lhs 左
/// @param rhs 右
/// @return lhs 是否应排在 rhs 前面
bool TickMgr::shouldRunBefore(const TickMgr::TickRecord& lhs, const TickMgr::TickRecord& rhs) const {
    if (lhs.nextFireMs != rhs.nextFireMs) {
        return lhs.nextFireMs < rhs.nextFireMs;
    }
    return lhs.listener < rhs.listener;
}

/// @brief 判断当前是否存在正在分发的任务
/// @return 状态
bool TickMgr::isDispatching() const {
    return mDispatchingRecord != nullptr;
}

/// @brief 获取等待列表首项
/// @return 任务记录；列表为空或首项异常时返回 nullptr
TickMgr::TickRecord* TickMgr::peekFront() const {
    if (mListeners.empty()) {
        return nullptr;
    }
    if (!mListeners.front()) {
        LOGE("peekFront failed: mListeners.front() is nullptr");
        return nullptr;
    }
    return mListeners.front();
}

/// @brief 获取当前正在执行的任务记录
/// @param listener 任务指针
/// @return 记录指针；未命中返回 nullptr
TickMgr::TickRecord* TickMgr::findDispatchingRecord(const ITickClass* listener) const {
    if (mDispatchingRecord && mDispatchingRecord->listener == listener) {
        return mDispatchingRecord;
    }
    return nullptr;
}

/// @brief 查找指定任务在等待调度列表中的位置
/// @param listener 任务指针
/// @return 下标；未找到时返回 kInvalidIndex
size_t TickMgr::findScheduledIndex(const ITickClass* listener) const {
    for (size_t i = 0; i < mListeners.size(); ++i) {
        if (mListeners[i] && mListeners[i]->listener == listener) {
            return i;
        }
    }
    return kInvalidIndex;
}

/// @brief 判断任务是否仍处于注册状态
/// @param listener 任务指针
/// @return 状态
bool TickMgr::isRegistered(const ITickClass* listener) const {
    if (!listener) {
        return false;
    }
    if (findScheduledIndex(listener) != kInvalidIndex) {
        return true;
    }
    TickRecord* dispatching = findDispatchingRecord(listener);
    return dispatching && !dispatching->cancelled;
}

/// @brief 从缓存池中获取一个 TickRecord
/// @return TickRecord 指针
TickMgr::TickRecord* TickMgr::acquireRecord() {
    if (!mRecordPool.empty()) {
        TickRecord* record = mRecordPool.back();
        mRecordPool.pop_back();
        if (!record) {
            LOGE("acquireRecord failed: nullptr record found in pool");
            return new TickRecord();
        }
        record->listener = nullptr;
        record->cancelled = false;
        record->intervalMs = 0;
        record->nextFireMs = 0;
        return record;
    }
    return new TickRecord();
}

/// @brief 回收一个 TickRecord 到缓存池
/// @param record TickRecord 指针
void TickMgr::recycleRecord(TickRecord* record) {
    if (!record) {
        return;
    }
    record->listener = nullptr;
    record->cancelled = false;
    record->intervalMs = 0;
    record->nextFireMs = 0;
    mRecordPool.push_back(record);
}

/// @brief 销毁一个 TickRecord
/// @param record TickRecord 指针
void TickMgr::destroyRecord(TickRecord* record) {
    delete record;
}

/// @brief 销毁一个 TickRecord 指针列表中的全部元素
/// @param records 列表
void TickMgr::destroyRecordList(std::vector<TickRecord*>& records) {
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i]) {
            destroyRecord(records[i]);
            records[i] = nullptr;
        }
    }
    records.clear();
}

/// @brief 将任务按 nextFireMs 有序插入等待调度列表
/// @param record 任务记录
void TickMgr::insertOrdered(TickRecord* record) {
    if (!record) {
        LOGE("insertOrdered failed: record is nullptr");
        return;
    }
    for (size_t i = 0; i < mListeners.size(); ++i) {
        if (!mListeners[i]) {
            LOGE("insertOrdered failed: nullptr record found in mListeners");
            return;
        }
        if (shouldRunBefore(*record, *mListeners[i])) {
            mListeners.insert(mListeners.begin() + static_cast<std::ptrdiff_t>(i), record);
            return;
        }
    }
    mListeners.push_back(record);
}

/// @brief 从等待调度列表中提取指定任务记录
/// @param listener 任务指针
/// @return 任务记录；未找到时返回 nullptr
TickMgr::TickRecord* TickMgr::extractScheduledRecord(const ITickClass* listener) {
    const size_t index = findScheduledIndex(listener);
    if (index == kInvalidIndex) {
        return nullptr;
    }
    TickRecord* record = mListeners[index];
    mListeners.erase(mListeners.begin() + static_cast<std::ptrdiff_t>(index));
    return record;
}

/// @brief 提取首个已到期任务记录
/// @param nowMs 当前时间
/// @return 任务记录；无到期任务时返回 nullptr
TickMgr::TickRecord* TickMgr::extractFrontIfDue(int64_t nowMs) {
    TickRecord* front = peekFront();
    if (!front || front->nextFireMs > nowMs) {
        return nullptr;
    }
    TickRecord* record = mListeners.front();
    mListeners.erase(mListeners.begin());
    return record;
}

/// @brief 开始一次任务分发
/// @param record 任务记录
/// @param nowMs 当前时间
/// @return 状态
/// @details 将当前任务移交到 mDispatchingRecord，并将下次触发时间推进到 nowMs + intervalMs
bool TickMgr::beginDispatch(TickRecord* record, int64_t nowMs) {
    if (!record) {
        LOGE("beginDispatch failed: record is nullptr");
        return false;
    }
    if (isDispatching()) {
        LOGE("beginDispatch failed: another record is dispatching, current=%p, new=%p",
            mDispatchingRecord->listener, record->listener);
        return false;
    }
    record->cancelled = false;
    record->nextFireMs = nowMs + record->intervalMs;
    mDispatchingRecord = record;
    return true;
}

/// @brief 完成一次任务分发
void TickMgr::finishDispatch() {
    if (!mDispatchingRecord) {
        LOGE("finishDispatch failed: mDispatchingRecord is nullptr");
        return;
    }

    TickRecord* record = mDispatchingRecord;
    mDispatchingRecord = nullptr;

    if (record->cancelled) {
        recycleRecord(record);
        return;
    }
    insertOrdered(record);
}

/// @brief 执行一次任务分发
/// @param record 任务记录
/// @param nowMs 当前时间
/// @return 状态
/// @details beginDispatch() -> onTick() -> finishDispatch()
bool TickMgr::dispatchRecord(TickRecord* record, int64_t nowMs) {
    if (!record) {
        LOGE("dispatchRecord failed: record is nullptr");
        return false;
    }

    ITickClass* callbackTarget = record->listener;
    const int64_t intervalMs = record->intervalMs;
    if (!callbackTarget) {
        LOGE("dispatchRecord failed: callbackTarget is nullptr");
        recycleRecord(record);
        return false;
    }

    if (!beginDispatch(record, nowMs)) {
        LOGE("dispatchRecord failed: beginDispatch failed, this=%p", callbackTarget);
        insertOrdered(record);
        return false;
    }

    const int64_t tickStartMs = getNowMs();
    callbackTarget->onTick(nowMs);
    const int64_t tickCostMs = getNowMs() - tickStartMs;
    if (tickCostMs > kTickWarnCostMs) {
        LOGW("dispatchRecord warning: onTick cost too long, this=%p, intervalMs=%lld, costMs=%lld",
            callbackTarget, (long long)intervalMs, (long long)tickCostMs);
    }

    finishDispatch();
    return true;
}
