/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-23 14:58:34
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
#include <memory>
#include <vector>

#include <core/looper.h>

#include "template/singleton.h"

#define g_tick TickMgr::instance()

/// @brief Tick 管理器
class TickMgr : public cdroid::EventHandler,
    public Singleton<TickMgr> {
    friend Singleton<TickMgr>;

public:
    /// @brief Tick 监听接口
    /// @note 直接调用 startTick 默认周期是 1 秒
    class ITickListener {
    private:
        int64_t mTickIntervalMs{ 1000 };

    public:
        virtual ~ITickListener();
        virtual void onTick(int64_t nowMs) = 0;

    protected:
        void setTick(int64_t intervalMs);
        void startTick(int64_t firstDelayMs = 0);
        void stopTick();
    };

protected:
    TickMgr();
    virtual ~TickMgr();

public:
    void   init();
    size_t size() const;
    void   clear();

    bool   addTick(ITickListener* listener, int64_t intervalMs, int64_t firstDelayMs = -1);
    bool   removeTick(ITickListener* listener);
    bool   hasTick(const ITickListener* listener) const;

    bool   runTickNow(ITickListener* listener);
    bool   updateInterval(ITickListener* listener, int64_t intervalMs, bool restartFromNow = true);

protected:
    virtual int checkEvents() override;
    virtual int handleEvents() override;

private:
    struct TickRecord {
        ITickListener* listener{ nullptr };    // 回调对象
        bool           cancelled{ false };     // 取消标识
        int64_t        intervalMs{ 0 };        // Tick 周期
        int64_t        nextFireMs{ 0 };        // 下次触发时间
    };

    typedef std::unique_ptr<TickRecord>                TickRecordPtr;
    typedef std::vector<TickRecordPtr>::iterator       ListenerIterator;
    typedef std::vector<TickRecordPtr>::const_iterator ConstListenerIterator;

private:
    bool shouldRunBefore(const TickRecord& lhs, const TickRecord& rhs) const;
    bool isDispatching() const;

    TickRecord*           peekFront();
    const TickRecord*     peekFront() const;

    ListenerIterator      findScheduledRecord(ITickListener* listener);
    ConstListenerIterator findScheduledRecord(const ITickListener* listener) const;

    TickRecord*           findDispatchingRecord(ITickListener* listener);
    const TickRecord*     findDispatchingRecord(const ITickListener* listener) const;

    bool isRegistered(const ITickListener* listener) const;

    void          insertOrdered(TickRecordPtr record);
    TickRecordPtr extractScheduledRecord(ITickListener* listener);
    TickRecordPtr extractFrontIfDue(int64_t nowMs);

    bool beginDispatch(TickRecordPtr record, int64_t nowMs);
    void finishDispatch();
    bool dispatchRecord(TickRecordPtr record, int64_t nowMs);

private:
    TickRecordPtr              mDispatchingRecord;   // 当前正在执行 onTick 的任务，脱离 mListeners 独立持有
    std::vector<TickRecordPtr> mListeners;           // 已注册任务，按 nextFireMs 从小到大有序
};

#endif // __TICK_MGR_H__
