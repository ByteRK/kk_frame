/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:44
 * @LastEditTime: 2026-07-04 21:30:00
 * @FilePath: /kk_frame/src/app/managers/timer_mgr.h
 * @Description: 有限生命周期定时任务管理器
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TIMER_MGR_H__
#define __TIMER_MGR_H__

#include <stdint.h>
#include <stddef.h>
#include <functional>
#include <map>
#include <vector>

#include <core/looper.h>

#include "template/singleton.h"

#define g_timer TimerMgr::instance()

/// @brief 有限生命周期定时任务管理器
/// @note 持续、无限次的周期心跳应使用 TickMgr
/// @note 管理器初始化、任务创建和任务取消应在所属 Looper 线程执行，不保证跨线程安全
class TimerMgr : public Singleton<TimerMgr>,
    public cdroid::EventHandler {
    friend class Singleton<TimerMgr>;

public:
    /// @brief 定时任务唯一标识；0 表示无效标识
    using TimerId = uint32_t;

    /// @brief 定时任务回调
    /// @param id 当前任务标识
    /// @param count 当前执行次数，从 1 开始
    using Callback = std::function<void(TimerId, uint32_t)>;

    /// @brief 自动取消定时任务的移动句柄
    /// @note 句柄不可复制，只能移动；有效句柄析构时会自动取消对应任务
    /// @note scopedAfter/scopedRepeated 的返回值必须保存，临时句柄会在语句结束时立即取消任务
    class TimerHandle {
        friend class TimerMgr;
    public:
        /// @brief 构造一个无效句柄
        TimerHandle();

        /// @brief 析构句柄；如果任务仍然有效则自动取消
        ~TimerHandle();

        /// @brief 移动构造；源句柄转为无效状态
        /// @param other 源句柄
        TimerHandle(TimerHandle&& other);

        /// @brief 移动赋值；先取消当前任务，再接管源句柄
        /// @param other 源句柄
        /// @return 当前句柄
        TimerHandle& operator=(TimerHandle&& other);

        TimerHandle(const TimerHandle&) = delete;
        TimerHandle& operator=(const TimerHandle&) = delete;

        /// @brief 取消关联任务并将句柄置为无效
        /// @note 可重复调用；任务已结束时仅清空句柄
        void cancel();

        /// @brief 判断关联任务当前是否仍在管理器中
        /// @return 任务仍处于等待或回调状态时返回 true
        bool isActive() const;

        /// @brief 获取关联任务标识
        /// @return 任务标识；无效句柄返回 0
        TimerId id() const { return mId; }

    private:
        /// @brief 由 TimerMgr 创建一个关联指定任务的句柄
        TimerHandle(TimerMgr* owner, TimerId id);

    private:
        TimerMgr* mOwner{ nullptr }; // 所属管理器
        TimerId   mId{ 0 };          // 关联任务标识
    };

protected:
    /// @brief 构造定时任务管理器
    TimerMgr();

public:
    /// @brief 析构管理器，移除事件处理器并释放全部任务记录
    ~TimerMgr();

    /// @brief 在当前线程的 Looper 上初始化管理器
    /// @note 可重复调用；首次成功初始化后后续调用无效
    void init();

    /// @brief 获取当前有效任务数量
    /// @return 等待执行和正在回调的任务总数
    size_t size() const;

    /// @brief 取消全部任务
    /// @note 正在回调的任务会被标记取消，并在回调结束后回收
    void cancelAll();

    /// @brief 延迟执行一次
    /// @param delayMs 延迟时间，单位毫秒，必须大于 0
    /// @param callback 到期回调
    /// @return 成功返回非 0 任务标识，失败返回 0
    TimerId runAfter(int64_t delayMs, Callback callback);

    /// @brief 有限次数重复执行
    /// @param intervalMs 后续执行间隔，单位毫秒，必须大于 0
    /// @param repeatCount 总执行次数，必须大于 0；无限周期任务应使用 TickMgr
    /// @param callback 每次到期时执行的回调
    /// @param firstDelayMs 首次延迟，单位毫秒；小于 0 时使用 intervalMs，0 表示下一轮事件处理时执行
    /// @return 成功返回非 0 任务标识，失败返回 0
    TimerId runRepeated(int64_t intervalMs, uint32_t repeatCount,
        Callback callback, int64_t firstDelayMs = -1);

    /// @brief 创建由句柄管理的一次性延迟任务
    /// @param delayMs 延迟时间，单位毫秒，必须大于 0
    /// @param callback 到期回调
    /// @return 自动取消句柄；创建失败时返回无效句柄
    /// @note 调用方必须保存返回值，否则临时句柄析构会立即取消任务
    TimerHandle scopedAfter(int64_t delayMs, Callback callback);

    /// @brief 创建由句柄管理的有限次数重复任务
    /// @param intervalMs 后续执行间隔，单位毫秒，必须大于 0
    /// @param repeatCount 总执行次数，必须大于 0
    /// @param callback 每次到期时执行的回调
    /// @param firstDelayMs 首次延迟；小于 0 时使用 intervalMs
    /// @return 自动取消句柄；创建失败时返回无效句柄
    /// @note 调用方必须保存返回值，否则临时句柄析构会立即取消任务
    TimerHandle scopedRepeated(int64_t intervalMs, uint32_t repeatCount,
        Callback callback, int64_t firstDelayMs = -1);

    /// @brief 根据任务标识取消定时任务
    /// @param id 任务标识
    /// @return 找到并取消任务返回 true，否则返回 false
    bool cancel(TimerId id);

    /// @brief 判断任务是否仍然有效
    /// @param id 任务标识
    /// @return 任务处于等待或回调状态时返回 true
    bool contains(TimerId id) const;

protected:
    /// @brief 检查等待队列首项是否到期
    /// @return 存在到期任务时返回 1，否则返回 0
    int checkEvents() override;

    /// @brief 依次执行本轮所有已到期任务
    /// @return 本轮实际执行的任务数量
    int handleEvents() override;

private:
    /// @brief 单个定时任务的内部调度记录
    struct TimerRecord {
        TimerId  id{ 0 };             // 任务标识
        Callback callback;            // 到期回调
        bool     cancelled{ false };  // 回调期间的取消标识
        int64_t  intervalMs{ 0 };     // 后续执行间隔
        int64_t  nextFireMs{ 0 };     // 下次触发的 uptime 时间
        uint32_t repeatCount{ 0 };    // 总执行次数
        uint32_t firedCount{ 0 };     // 已执行次数
    };

private:
    /// @brief 分配当前未被占用的非 0 任务标识
    TimerId allocateId();

    /// @brief 从缓存池获取并重置一条任务记录
    TimerRecord* acquireRecord();

    /// @brief 重置任务记录并放回缓存池
    void recycleRecord(TimerRecord* record);

    /// @brief 按触发时间和任务标识将记录插入等待队列
    void insertOrdered(TimerRecord* record);

    /// @brief 从等待队列提取首个已到期任务
    /// @param nowMs 当前 uptime 时间
    /// @return 已到期记录；没有到期任务时返回 nullptr
    TimerRecord* extractFrontIfDue(int64_t nowMs);

    /// @brief 执行单条任务记录
    /// @param record 已从等待队列移出的任务记录
    /// @param nowMs 本轮调度时间
    /// @return 执行成功返回 true
    bool dispatchRecord(TimerRecord* record, int64_t nowMs);

    /// @brief 完成当前回调，根据取消状态和执行次数回收或重新调度记录
    void finishDispatch();

private:
    bool                            mInited{ false };             // 是否初始化成功
    TimerId                         mNextId{ 0 };                 // 上次分配的任务标识
    cdroid::Looper*                 mLooper{ nullptr };           // 所属线程的 Looper
    TimerRecord*                    mDispatchingRecord{ nullptr };// 当前正在回调的记录
    std::vector<TimerRecord*>       mScheduled;                   // 按 nextFireMs 排序的等待队列
    std::vector<TimerRecord*>       mRecordPool;                  // 空闲任务记录缓存池
    std::map<TimerId, TimerRecord*> mRecords;                     // 有效任务索引，包含正在回调的任务
};

#endif // __TIMER_MGR_H__
