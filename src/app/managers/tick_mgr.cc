/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-23 01:47:26
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
/// @details
/// 若对象析构前未先从 TickMgr 中移除，会打印错误日志，便于开发阶段排查生命周期问题。
ITickListener::~ITickListener() {
    if (mTickTask.registered ||
        mTickTask.dispatching ||
        mTickTask.intervalMs != 0 ||
        mTickTask.nextFireMs != 0) {
        LOGE("ITickListener destroyed without removeTick first: this=%p, registered=%d, dispatching=%d, intervalMs=%lld, nextFireMs=%lld",
            this,
            (int)mTickTask.registered,
            (int)mTickTask.dispatching,
            (long long)mTickTask.intervalMs,
            (long long)mTickTask.nextFireMs);
    }
}

/// @brief 构造函数
TickMgr::TickMgr()
    : mDispatchingListener(NULL) { }

/// @brief 析构函数
/// @details 析构时先从主线程 Looper 反注册，再清空全部 Tick 任务。
TickMgr::~TickMgr() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
    clear();
}

/// @brief 初始化 TickMgr
/// @details 将自身注册到主线程 Looper，使主循环能够通过 checkEvents/handleEvents 驱动 Tick 调度。
void TickMgr::init() {
    cdroid::Looper::getMainLooper()->addEventHandler(this);
}

/// @brief 获取已注册任务数量
/// @return 数量
/// @details
/// mListeners 里保存的是“等待调度”的任务；若当前正有一个任务处于 dispatching 中，
/// 它在这一小段时间里不在 mListeners 中，但仍属于已注册任务，因此需要额外计数。
size_t TickMgr::size() const {
    TICK_MGR_LOCK_GUARD();

    size_t count = mListeners.size();
    if (mDispatchingListener != NULL && mDispatchingListener->mTickTask.registered) {
        ++count;
    }
    return count;
}

/// @brief 清除所有任务
/// @details
/// 会清空等待调度队列，并将所有 listener 的 TickTask 状态复位。
/// 若当前正有任务处于 onTick 中，则仅将其标记为 registered=false，
/// 待回调返回后由 finishDispatch() 完成最终复位，避免在回调栈尚未结束时破坏其过程态。
void TickMgr::clear() {
    TICK_MGR_LOCK_GUARD();

    for (size_t i = 0; i < mListeners.size(); ++i) {
        if (mListeners[i] == NULL) {
            LOGE("clear warning: mListeners[%zu] is NULL", i);
            continue;
        }
        resetTickTask(mListeners[i]);
    }
    mListeners.clear();

    if (mDispatchingListener != NULL) {
        mDispatchingListener->mTickTask.registered = false;
    }
}

/// @brief 添加任务
/// @param listener 任务指针
/// @param intervalMs 间隔时间，必须大于 0
/// @param firstDelayMs 首次延迟时间，小于 0 时默认等于 intervalMs
/// @return 状态
/// @details
/// 一个 listener 同一时刻只允许注册一个 Tick 任务。
/// 注册成功后，任务会立即进入有序调度列表，并持续执行，除非 removeTick()/clear() 被调用。
bool TickMgr::addTick(ITickListener* listener,
    int64_t intervalMs,
    int64_t firstDelayMs) {
    if (listener == NULL) {
        LOGE("addTick failed: listener is NULL");
        return false;
    }

    if (intervalMs <= 0) {
        LOGE("addTick failed: invalid intervalMs=%lld", (long long)intervalMs);
        return false;
    }

    const int64_t nowMs = getNowMs();
    const int64_t delayMs = (firstDelayMs >= 0) ? firstDelayMs : intervalMs;

    TICK_MGR_LOCK_GUARD();

    if (isRegistered(listener)) {
        LOGE("addTick failed: listener already exists, this=%p", listener);
        return false;
    }

    listener->mTickTask.registered = true;
    listener->mTickTask.dispatching = false;
    listener->mTickTask.intervalMs = intervalMs;
    listener->mTickTask.nextFireMs = nowMs + delayMs;

    insertOrdered(listener);
    return true;
}

/// @brief 移除任务
/// @param listener 任务指针
/// @return 状态
/// @details
/// 若任务当前处于等待调度列表中，会立即从列表移除并复位。
/// 若任务当前正处于 onTick 执行中，则只会先标记为未注册，
/// 待回调返回后由 finishDispatch() 收尾，不再重新插回调度列表。
bool TickMgr::removeTick(ITickListener* listener) {
    if (listener == NULL) {
        LOGE("removeTick failed: listener is NULL");
        return false;
    }

    TICK_MGR_LOCK_GUARD();

    if (!isRegistered(listener)) {
        LOGE("removeTick failed: listener not found, this=%p", listener);
        return false;
    }

    if (listener->mTickTask.dispatching) {
        if (mDispatchingListener != listener) {
            LOGE("removeTick failed: dispatching state mismatch, this=%p, mDispatchingListener=%p",
                listener, mDispatchingListener);
            return false;
        }

        listener->mTickTask.registered = false;
        return true;
    }

    if (!eraseScheduledListener(listener)) {
        LOGE("removeTick failed: scheduled listener not found, this=%p", listener);
        return false;
    }

    resetTickTask(listener);
    return true;
}

/// @brief 判断任务是否已注册
/// @param listener 任务指针
/// @return 状态
bool TickMgr::hasTick(const ITickListener* listener) const {
    if (listener == NULL) {
        LOGE("hasTick failed: listener is NULL");
        return false;
    }

    TICK_MGR_LOCK_GUARD();
    return isRegistered(listener);
}

/// @brief 复位任务状态
/// @param listener 任务指针
/// @details 将 listener 内保存的 TickTask 完全恢复为初始状态。
void TickMgr::resetTickTask(ITickListener* listener) {
    if (listener == NULL) {
        LOGE("resetTickTask failed: listener is NULL");
        return;
    }

    listener->mTickTask.registered = false;
    listener->mTickTask.dispatching = false;
    listener->mTickTask.intervalMs = 0;
    listener->mTickTask.nextFireMs = 0;
}

/// @brief 判断任务是否仍处于注册状态
/// @param listener 任务指针
/// @return 状态
bool TickMgr::isRegistered(const ITickListener* listener) const {
    return listener != NULL && listener->mTickTask.registered;
}

/// @brief 比较两个任务的调度优先级
/// @param lhs 左任务
/// @param rhs 右任务
/// @return lhs 是否应排在 rhs 前面
/// @details
/// 先按 nextFireMs 从小到大排序；若时间相同，则按指针地址打破平局，
/// 避免同一时刻任务在有序表中的相对顺序频繁抖动。
bool TickMgr::shouldRunBefore(const ITickListener* lhs, const ITickListener* rhs) const {
    if (lhs == NULL || rhs == NULL) {
        LOGE("shouldRunBefore failed: lhs=%p, rhs=%p", lhs, rhs);
        return false;
    }

    if (lhs->mTickTask.nextFireMs != rhs->mTickTask.nextFireMs) {
        return lhs->mTickTask.nextFireMs < rhs->mTickTask.nextFireMs;
    }

    return lhs < rhs;
}

/// @brief 查找指定任务在等待调度列表中的位置
/// @param listener 任务指针
/// @return 迭代器；未找到则返回 end()
TickMgr::ListenerIterator TickMgr::findListener(ITickListener* listener) {
    for (ListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (*it == listener) {
            return it;
        }
    }
    return mListeners.end();
}

/// @brief 查找指定任务在等待调度列表中的位置（const 版本）
/// @param listener 任务指针
/// @return 常量迭代器；未找到则返回 end()
TickMgr::ConstListenerIterator TickMgr::findListener(const ITickListener* listener) const {
    for (ConstListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (*it == listener) {
            return it;
        }
    }
    return mListeners.end();
}

/// @brief 将任务按 nextFireMs 有序插入等待调度列表
/// @param listener 任务指针
/// @details
/// 当前实现使用 vector + 线性查找插入位。
/// 由于 checkEvents() 只关注首项，且通常任务量不会特别大，这种方式比额外维护小顶堆更直观。
void TickMgr::insertOrdered(ITickListener* listener) {
    if (listener == NULL) {
        LOGE("insertOrdered failed: listener is NULL");
        return;
    }

    for (ListenerIterator it = mListeners.begin(); it != mListeners.end(); ++it) {
        ITickListener* current = *it;
        if (current == NULL) {
            LOGE("insertOrdered failed: NULL listener found in mListeners");
            return;
        }

        if (shouldRunBefore(listener, current)) {
            mListeners.insert(it, listener);
            return;
        }
    }

    mListeners.push_back(listener);
}

/// @brief 从等待调度列表中移除指定任务
/// @param listener 任务指针
/// @return 状态
bool TickMgr::eraseScheduledListener(ITickListener* listener) {
    ListenerIterator it = findListener(listener);
    if (it == mListeners.end()) {
        return false;
    }

    mListeners.erase(it);
    return true;
}

/// @brief 开始一次任务分发
/// @param listener 任务指针
/// @param nowMs 当前时间
/// @return 状态
/// @details
/// 该接口会先将任务从等待调度列表中摘除，标记为 dispatching，
/// 并按 fixed-delay 策略将 nextFireMs 推进到 nowMs + intervalMs。
/// 回调结束后必须配对调用 finishDispatch() 收尾。
bool TickMgr::beginDispatch(ITickListener* listener, int64_t nowMs) {
    if (listener == NULL) {
        LOGE("beginDispatch failed: listener is NULL");
        return false;
    }

    if (!isRegistered(listener)) {
        LOGE("beginDispatch failed: listener not registered, this=%p", listener);
        return false;
    }

    if (listener->mTickTask.dispatching) {
        LOGE("beginDispatch failed: listener already dispatching, this=%p", listener);
        return false;
    }

    if (mDispatchingListener != NULL) {
        LOGE("beginDispatch failed: another listener is dispatching, current=%p, new=%p",
            mDispatchingListener, listener);
        return false;
    }

    if (!eraseScheduledListener(listener)) {
        LOGE("beginDispatch failed: scheduled listener not found, this=%p", listener);
        return false;
    }

    listener->mTickTask.dispatching = true;
    listener->mTickTask.nextFireMs = nowMs + listener->mTickTask.intervalMs;
    mDispatchingListener = listener;
    return true;
}

/// @brief 完成一次任务分发
/// @param listener 任务指针
/// @details
/// 若任务在回调期间仍保持 registered=true，则按 nextFireMs 重新有序插回等待调度列表；
/// 若已被 removeTick()/clear() 取消注册，则直接复位状态，不再重新插入。
void TickMgr::finishDispatch(ITickListener* listener) {
    if (listener == NULL) {
        LOGE("finishDispatch failed: listener is NULL");
        return;
    }

    if (!listener->mTickTask.dispatching) {
        LOGE("finishDispatch failed: listener not dispatching, this=%p", listener);
        return;
    }

    if (mDispatchingListener != listener) {
        LOGE("finishDispatch failed: dispatching listener mismatch, this=%p, mDispatchingListener=%p",
            listener, mDispatchingListener);
        return;
    }

    listener->mTickTask.dispatching = false;
    mDispatchingListener = NULL;

    if (!listener->mTickTask.registered) {
        resetTickTask(listener);
        return;
    }

    insertOrdered(listener);
}

/// @brief 更新任务周期
/// @param listener 任务指针
/// @param intervalMs 新周期，必须大于 0
/// @param restartFromNow 是否从当前时刻重新计时
/// @return 状态
/// @details
/// - 若任务当前处于等待调度列表中，更新后会重新按 nextFireMs 插入有序位置。
/// - 若任务当前正在执行 onTick，则只修改其 TickTask；待回调结束后由 finishDispatch() 重新插回列表。
bool TickMgr::updateInterval(ITickListener* listener,
    int64_t intervalMs,
    bool restartFromNow) {
    if (listener == NULL) {
        LOGE("updateInterval failed: listener is NULL");
        return false;
    }

    if (intervalMs <= 0) {
        LOGE("updateInterval failed: invalid intervalMs=%lld", (long long)intervalMs);
        return false;
    }

    TICK_MGR_LOCK_GUARD();

    if (!isRegistered(listener)) {
        LOGE("updateInterval failed: listener not found, this=%p", listener);
        return false;
    }

    listener->mTickTask.intervalMs = intervalMs;
    if (restartFromNow) {
        listener->mTickTask.nextFireMs = getNowMs() + intervalMs;
    }

    if (listener->mTickTask.dispatching) {
        return true;
    }

    if (!eraseScheduledListener(listener)) {
        LOGE("updateInterval failed: scheduled listener not found, this=%p", listener);
        return false;
    }

    insertOrdered(listener);
    return true;
}

/// @brief 立即执行指定任务一次
/// @param listener 任务指针
/// @return 状态
/// @details
/// 该接口会立刻调用一次 listener->onTick(nowMs)，同时把 nextFireMs 推进到 nowMs + intervalMs，
/// 避免该任务在本次立即执行后又被下一轮 checkEvents/handleEvents 立刻命中。
///
/// 行为约定：
/// - 仅要求任务已注册
/// - 不允许对当前已处于 dispatching 状态的任务再次 runTickNow，避免重入导致状态紊乱
/// - 若回调期间调用了 removeTick/clear/restartTick/updateInterval 等接口，以回调结束后的真实状态为准
bool TickMgr::runTickNow(ITickListener* listener) {
    if (listener == NULL) {
        LOGE("runTickNow failed: listener is NULL");
        return false;
    }

    const int64_t nowMs = getNowMs();

    {
        TICK_MGR_LOCK_GUARD();

        if (!isRegistered(listener)) {
            LOGE("runTickNow failed: listener not found, this=%p", listener);
            return false;
        }

        if (!beginDispatch(listener, nowMs)) {
            LOGE("runTickNow failed: beginDispatch failed, this=%p", listener);
            return false;
        }
    }

    listener->onTick(nowMs);

    {
        TICK_MGR_LOCK_GUARD();
        finishDispatch(listener);
    }

    return true;
}

/// @brief 重启任务计时
/// @param listener 任务指针
/// @param delayMs 延迟时间，小于 0 时默认等于 intervalMs
/// @return 状态
/// @details
/// 该接口会按新的 nextFireMs 重新参与调度。
/// 若当前处于 dispatching 中，则仅修改 nextFireMs，待回调结束后再统一插回。
bool TickMgr::restartTick(ITickListener* listener, int64_t delayMs) {
    if (listener == NULL) {
        LOGE("restartTick failed: listener is NULL");
        return false;
    }

    TICK_MGR_LOCK_GUARD();

    if (!isRegistered(listener)) {
        LOGE("restartTick failed: listener not found, this=%p", listener);
        return false;
    }

    const int64_t actualDelayMs =
        (delayMs >= 0) ? delayMs : listener->mTickTask.intervalMs;

    listener->mTickTask.nextFireMs = getNowMs() + actualDelayMs;

    if (listener->mTickTask.dispatching) {
        return true;
    }

    if (!eraseScheduledListener(listener)) {
        LOGE("restartTick failed: scheduled listener not found, this=%p", listener);
        return false;
    }

    insertOrdered(listener);
    return true;
}

/// @brief 检查是否存在到期任务
/// @return 非 0 表示存在到期任务
/// @details
/// 当前 mListeners 始终按 nextFireMs 有序，因此这里只需检查首项是否到期，
/// 无需每次遍历全部 listener。
int TickMgr::checkEvents() {
    const int64_t nowMs = getNowMs();

    TICK_MGR_LOCK_GUARD();
    if (mListeners.empty()) {
        return 0;
    }

    ITickListener* listener = mListeners.front();
    if (listener == NULL) {
        LOGE("checkEvents failed: mListeners.front() is NULL");
        return 0;
    }

    return (listener->mTickTask.nextFireMs <= nowMs) ? 1 : 0;
}

/// @brief 执行全部到期任务
/// @return 本轮实际执行的任务数量
/// @details
/// 采用 fixed-delay 策略：本轮任务执行前，先把 nextFireMs 推进到 nowMs + intervalMs。
/// 这样即使某个回调执行时间较长，也不会在下一轮中补偿触发多次。
///
/// 回调期间允许任务修改自身状态：
/// - removeTick(this)
/// - restartTick(this)
/// - updateInterval(this, ...)
/// - clear()
///
/// 因此流程设计为：
/// 1. 从有序列表首项摘除到期任务
/// 2. 标记 dispatching 并执行回调
/// 3. 回调结束后，根据最终状态决定是否重新有序插回列表
int TickMgr::handleEvents() {
    const int64_t nowMs = getNowMs();
    int firedCount = 0;

    while (true) {
        ITickListener* listener = NULL;

        {
            TICK_MGR_LOCK_GUARD();

            if (mListeners.empty()) {
                break;
            }

            listener = mListeners.front();
            if (listener == NULL) {
                LOGE("handleEvents failed: mListeners.front() is NULL");
                mListeners.erase(mListeners.begin());
                continue;
            }

            if (listener->mTickTask.nextFireMs > nowMs) {
                break;
            }

            if (!beginDispatch(listener, nowMs)) {
                LOGE("handleEvents failed: beginDispatch failed, this=%p", listener);
                break;
            }
        }

        listener->onTick(nowMs);
        ++firedCount;

        {
            TICK_MGR_LOCK_GUARD();
            finishDispatch(listener);
        }
    }

    return firedCount;
}
