/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-23 11:20:06
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

/// @brief 获取当前时间戳
/// @return 毫秒时间戳
static int64_t getNowMs() {
    return cdroid::SystemClock::uptimeMillis();
}

/// @brief Tick 监听对象析构
TickMgr::ITickListener::~ITickListener() { }

/// @brief Tick 管理器构造
TickMgr::TickMgr() { }

/// @brief Tick 管理器析构
TickMgr::~TickMgr() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
    clear();
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
    mListeners.clear();
    if (mDispatchingRecord) {
        mDispatchingRecord->cancelled = true;
    }
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
    return static_cast<bool>(mDispatchingRecord);
}

/// @brief 获取等待列表首项
/// @return 任务记录；列表为空或首项异常时返回 nullptr
TickMgr::TickRecord* TickMgr::peekFront() {
    if (mListeners.empty()) {
        return nullptr;
    }
    if (!mListeners.front()) {
        LOGE("peekFront failed: mListeners.front() is nullptr");
        return nullptr;
    }
    return mListeners.front().get();
}

/// @brief 获取等待列表首项（const 版本）
/// @return 任务记录；列表为空或首项异常时返回 nullptr
const TickMgr::TickRecord* TickMgr::peekFront() const {
    if (mListeners.empty()) {
        return nullptr;
    }
    if (!mListeners.front()) {
        LOGE("peekFront failed: mListeners.front() is nullptr");
        return nullptr;
    }
    return mListeners.front().get();
}

/// @brief 查找指定任务在等待调度列表中的位置
/// @param listener 任务指针
/// @return 迭代器；未找到则返回 end()
TickMgr::ListenerIterator TickMgr::findScheduledRecord(ITickListener* listener) {
    for (ListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (*it && (*it)->listener == listener) {
            return it;
        }
    }
    return mListeners.end();
}

/// @brief 查找指定任务在等待调度列表中的位置（const 版本）
/// @param listener 任务指针
/// @return 常量迭代器；未找到则返回 end()
TickMgr::ConstListenerIterator TickMgr::findScheduledRecord(const ITickListener* listener) const {
    for (ConstListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (*it && (*it)->listener == listener) {
            return it;
        }
    }
    return mListeners.end();
}

/// @brief 获取当前正在执行的任务记录
/// @param listener 任务指针
/// @return 记录指针；未命中返回 nullptr
TickMgr::TickRecord* TickMgr::findDispatchingRecord(ITickListener* listener) {
    if (mDispatchingRecord && mDispatchingRecord->listener == listener) {
        return mDispatchingRecord.get();
    }
    return nullptr;
}

/// @brief 获取当前正在执行的任务记录（const 版本）
/// @param listener 任务指针
/// @return 记录指针；未命中返回 nullptr
const TickMgr::TickRecord* TickMgr::findDispatchingRecord(const ITickListener* listener) const {
    if (mDispatchingRecord && mDispatchingRecord->listener == listener) {
        return mDispatchingRecord.get();
    }
    return nullptr;
}

/// @brief 判断任务是否仍处于注册状态
/// @param listener 任务指针
/// @return 状态
bool TickMgr::isRegistered(const ITickListener* listener) const {
    if (!listener) {
        return false;
    }
    if (findScheduledRecord(listener) != mListeners.end()) {
        return true;
    }
    const TickRecord* dispatching = findDispatchingRecord(listener);
    return dispatching && !dispatching->cancelled;
}

/// @brief 将任务按 nextFireMs 有序插入等待调度列表
/// @param record 任务记录所有权
void TickMgr::insertOrdered(TickRecordPtr record) {
    if (!record) {
        LOGE("insertOrdered failed: record is nullptr");
        return;
    }
    for (ListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (!(*it)) {
            LOGE("insertOrdered failed: nullptr record found in mListeners");
            return;
        }
        if (shouldRunBefore(*record, *(*it))) {
            mListeners.insert(it, std::move(record));
            return;
        }
    }
    mListeners.push_back(std::move(record));
}

/// @brief 从等待调度列表中提取指定任务记录
/// @param listener 任务指针
/// @return 任务记录所有权；未找到时返回空指针
TickMgr::TickRecordPtr TickMgr::extractScheduledRecord(ITickListener* listener) {
    ListenerIterator it = findScheduledRecord(listener);
    if (it == mListeners.end()) {
        return TickRecordPtr();
    }
    TickRecordPtr record = std::move(*it);
    mListeners.erase(it);
    return record;
}

/// @brief 提取首个已到期任务记录
/// @param nowMs 当前时间
/// @return 任务记录所有权；无到期任务时返回空指针
TickMgr::TickRecordPtr TickMgr::extractFrontIfDue(int64_t nowMs) {
    TickRecord* front = peekFront();
    if (!front || front->nextFireMs > nowMs) {
        return TickRecordPtr();
    }
    TickRecordPtr record = std::move(mListeners.front());
    mListeners.erase(mListeners.begin());
    return record;
}

/// @brief 开始一次任务分发
/// @param record 任务记录所有权
/// @param nowMs 当前时间
/// @return 状态
/// @details 将当前任务移交到 mDispatchingRecord
bool TickMgr::beginDispatch(TickRecordPtr record, int64_t nowMs) {
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
    mDispatchingRecord = std::move(record);
    return true;
}

/// @brief 完成一次任务分发
void TickMgr::finishDispatch() {
    if (!mDispatchingRecord) {
        LOGE("finishDispatch failed: mDispatchingRecord is nullptr");
        return;
    }
    if (mDispatchingRecord->cancelled) {
        mDispatchingRecord.reset();
        return;
    }
    insertOrdered(std::move(mDispatchingRecord));
}

/// @brief 执行一次任务分发
/// @param record 任务记录所有权
/// @param nowMs 当前时间
/// @return 状态
/// @details beginDispatch() -> onTick() -> finishDispatch()
bool TickMgr::dispatchRecord(TickRecordPtr record, int64_t nowMs) {
    if (!record) {
        LOGE("dispatchRecord failed: record is nullptr");
        return false;
    }
    ITickListener* callbackTarget = record->listener;
    if (!callbackTarget) {
        LOGE("dispatchRecord failed: callbackTarget is nullptr");
        return false;
    }
    if (!beginDispatch(std::move(record), nowMs)) {
        LOGE("dispatchRecord failed: beginDispatch failed, this=%p", callbackTarget);
        return false;
    }
    callbackTarget->onTick(nowMs);
    finishDispatch();
    return true;
}

/// @brief 添加任务
/// @param listener 任务指针
/// @param intervalMs 间隔时间，必须大于 0
/// @param firstDelayMs 首次延迟时间，小于 0 时默认等于 intervalMs
/// @return 状态
bool TickMgr::addTick(ITickListener* listener, int64_t intervalMs, int64_t firstDelayMs) {
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
    const int64_t delayMs = (firstDelayMs >= 0) ? firstDelayMs : intervalMs;

    TickRecordPtr record(new TickRecord());
    record->listener = listener;
    record->intervalMs = intervalMs;
    record->nextFireMs = nowMs + delayMs;
    insertOrdered(std::move(record));
    return true;
}

/// @brief 移除任务
/// @param listener 任务指针
/// @return 状态
bool TickMgr::removeTick(ITickListener* listener) {
    if (!listener) {
        LOGE("removeTick failed: listener is nullptr");
        return false;
    }
    TickRecordPtr record = extractScheduledRecord(listener);
    if (record) {
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
bool TickMgr::hasTick(const ITickListener* listener) const {
    if (!listener) {
        LOGE("hasTick failed: listener is nullptr");
        return false;
    }
    return isRegistered(listener);
}

/// @brief 立即执行指定任务一次
/// @param listener 任务指针
/// @return 状态
bool TickMgr::runTickNow(ITickListener* listener) {
    if (!listener) {
        LOGE("runTickNow failed: listener is nullptr");
        return false;
    }
    if (isDispatching()) {
        LOGE("runTickNow failed: another record is dispatching, this=%p, current=%p",
            listener, mDispatchingRecord->listener);
        return false;
    }
    TickRecordPtr record = extractScheduledRecord(listener);
    if (!record) {
        LOGE("runTickNow failed: listener not found in scheduled list, this=%p", listener);
        return false;
    }
    return dispatchRecord(std::move(record), getNowMs());
}

/// @brief 更新任务周期
/// @param listener 任务指针
/// @param intervalMs 新周期，必须大于 0
/// @param restartFromNow 是否从当前时刻重新计时
/// @return 状态
bool TickMgr::updateInterval(ITickListener* listener, int64_t intervalMs, bool restartFromNow) {
    if (!listener) {
        LOGE("updateInterval failed: listener is nullptr");
        return false;
    }
    if (intervalMs <= 0) {
        LOGE("updateInterval failed: invalid intervalMs=%lld", (long long)intervalMs);
        return false;
    }
    // 处于等待调度列表中
    TickRecordPtr record = extractScheduledRecord(listener);
    if (record) {
        record->intervalMs = intervalMs;
        if (restartFromNow) {
            record->nextFireMs = getNowMs() + intervalMs;
        }
        insertOrdered(std::move(record));
        return true;
    }
    // 正在执行 onTick
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
        TickRecordPtr record = extractFrontIfDue(nowMs);
        if (!record) {
            break;
        }
        if (!dispatchRecord(std::move(record), nowMs)) {
            LOGE("handleEvents failed: dispatchRecord failed");
            break;
        }
        ++firedCount;
    }
    return firedCount;
}
