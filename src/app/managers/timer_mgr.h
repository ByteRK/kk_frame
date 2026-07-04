/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:44
 * @LastEditTime: 2026-07-05 00:31:50
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

/// @brief 定时任务管理器
/// @note 适用于延迟回调任务、有限次执行任务
/// @note 持续、无限次的周期心跳使用 TickMgr
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

    /// @brief 自动挡任务句柄
    /// @note 句柄不可复制，只能移动，析构自动取消任务
    /// @note 调用方必须保存句柄，临时句柄会在语句结束时立即取消任务
    class TimerHandle {
        friend class TimerMgr;
    public:
        TimerHandle();
        ~TimerHandle();
    private:
        TimerHandle(TimerMgr* owner, TimerId id);
    public:
        TimerHandle(TimerHandle&& other);
        TimerHandle& operator=(TimerHandle&& other);
        TimerHandle(const TimerHandle&) = delete;
        TimerHandle& operator=(const TimerHandle&) = delete;
    public:
        void    cancel();
        bool    isActive() const;
        TimerId id() const;
    private:
        TimerMgr* mOwner{ nullptr }; // 所属管理器
        TimerId   mId{ 0 };          // 关联任务标识
    };

protected:
    TimerMgr();
    ~TimerMgr();

public:
    void        init();
    size_t      size() const;
    void        cancelAll();

    TimerId     runAfter(int64_t delayMs, Callback callback);
    TimerId     runRepeated(int64_t intervalMs, uint32_t repeatCount,
        Callback callback, int64_t firstDelayMs = -1);

    TimerHandle scopedAfter(int64_t delayMs, Callback callback);
    TimerHandle scopedRepeated(int64_t intervalMs, uint32_t repeatCount,
        Callback callback, int64_t firstDelayMs = -1);

    bool        cancel(TimerId id);
    bool        contains(TimerId id) const;

protected:
    int checkEvents()  override;
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
    TimerId      allocateId();
    TimerRecord* acquireRecord();

    void         recycleRecord(TimerRecord* record);
    void         insertOrdered(TimerRecord* record);

    TimerRecord* extractFrontIfDue(int64_t nowMs);
    bool         dispatchRecord(TimerRecord* record, int64_t nowMs);
    void         finishDispatch();

private:
    bool                            mInited{ false };             // 初始化标识
    TimerId                         mNextId{ 0 };                 // 上次分配的任务Id
    cdroid::Looper*                 mLooper{ nullptr };           // 所属 Looper
    TimerRecord*                    mDispatchingRecord{ nullptr };// 当前正在执行的任务，脱离 mScheduled
    std::vector<TimerRecord*>       mScheduled;                   // 已注册任务，按 nextFireMs 从小到大有序
    std::vector<TimerRecord*>       mRecordPool;                  // TimerRecord 缓存池
    std::map<TimerId, TimerRecord*> mRecords;                     // 有效任务索引，包含正在回调的任务
};

#endif // __TIMER_MGR_H__
