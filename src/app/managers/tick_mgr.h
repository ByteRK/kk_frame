/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-23 01:47:17
 * @FilePath: /kk_frame/src/app/managers/tick_mgr.h
 * @Description: Tick 管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TICK_MGR_H__
#define __TICK_MGR_H__

#include <stdint.h>
#include <stddef.h>
#include <vector>

#include <core/looper.h>

#include "template/singleton.h"

#ifndef TICK_MGR_ENABLE_THREAD_SAFE
#define TICK_MGR_ENABLE_THREAD_SAFE 0
#endif

#if TICK_MGR_ENABLE_THREAD_SAFE
#include <mutex>
#define TICK_MGR_LOCK_GUARD() std::lock_guard<std::mutex> lock(mMutex)
#else
#define TICK_MGR_LOCK_GUARD() ((void)0)
#endif

class TickMgr;

/// @brief Tick 监听接口
class ITickListener {
public:
    virtual ~ITickListener();
    virtual void onTick(int64_t nowMs) = 0;

private:
    struct TickTask {
        bool    registered{ false };   // 是否仍处于注册状态
        bool    dispatching{ false };  // 当前是否正在执行 onTick
        int64_t intervalMs{ 0 };       // Tick 周期
        int64_t nextFireMs{ 0 };       // 下次触发时间
    };

private:
    TickTask mTickTask;
    friend class TickMgr;
};

/// @brief Tick 管理器
class TickMgr : public cdroid::EventHandler,
    public Singleton<TickMgr> {
    friend Singleton<TickMgr>;

protected:
    TickMgr();
    virtual ~TickMgr();

public:
    void   init();
    size_t size() const;
    void   clear();

    bool addTick(ITickListener* listener,
        int64_t intervalMs,
        int64_t firstDelayMs = -1);
    bool removeTick(ITickListener* listener);
    bool hasTick(const ITickListener* listener) const;

    bool restartTick(ITickListener* listener, int64_t delayMs = -1);
    bool updateInterval(ITickListener* listener,
        int64_t intervalMs,
        bool restartFromNow = true);

    /// @brief 立即执行指定任务一次，并将下次触发时间推进到 now + intervalMs
    /// @param listener 任务指针
    /// @return 状态
    bool runTickNow(ITickListener* listener);

protected:
    virtual int checkEvents() override;
    virtual int handleEvents() override;

private:
    typedef std::vector<ITickListener*>::iterator ListenerIterator;
    typedef std::vector<ITickListener*>::const_iterator ConstListenerIterator;

private:
    void resetTickTask(ITickListener* listener);

    bool isRegistered(const ITickListener* listener) const;
    bool shouldRunBefore(const ITickListener* lhs, const ITickListener* rhs) const;

    ListenerIterator findListener(ITickListener* listener);
    ConstListenerIterator findListener(const ITickListener* listener) const;

    void insertOrdered(ITickListener* listener);
    bool eraseScheduledListener(ITickListener* listener);

    bool beginDispatch(ITickListener* listener, int64_t nowMs);
    void finishDispatch(ITickListener* listener);

private:
    std::vector<ITickListener*> mListeners; // 已注册且等待调度的任务，按 nextFireMs 从小到大有序
    ITickListener* mDispatchingListener;    // 当前正在执行 onTick 的任务；单线程模型下同一时刻只会有一个

#if TICK_MGR_ENABLE_THREAD_SAFE
    mutable std::mutex mMutex;
#endif
};

#endif // __TICK_MGR_H__
