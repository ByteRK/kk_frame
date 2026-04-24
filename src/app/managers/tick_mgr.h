/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-12 17:08:29
 * @LastEditTime: 2026-04-24 10:19:49
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

#define g_tick TickMgr::instance()

/// @brief Tick 管理器
class TickMgr : public cdroid::EventHandler,
    public Singleton<TickMgr> {
    friend Singleton<TickMgr>;

public:
    /// @brief Tick 任务类
    /// @note 直接调用 startTick 默认周期是 1 秒
    class ITickClass {
    private:
        int64_t mTickIntervalMs{ 1000 };
    public:
        virtual ~ITickClass();
        virtual void onTick(int64_t nowMs) = 0;
    protected:
        void setTick(int64_t intervalMs);
        void startTick(int64_t firstDelayMs = 0);
        void stopTick();
    };

    /// @brief Tick 任务变量类
    /// @note 便于不方便继承自 ITickClass 的类使用
    class ITickVariable : public ITickClass {
    public:
        using TickCallBack = std::function<void(int64_t)>;
    private:
        TickCallBack mTickCB;
    public:
        void setCallBack(TickCallBack cb) { mTickCB = cb; };
        void setTick(int64_t intervalMs) { ITickClass::setTick(intervalMs); };
        void startTick(int64_t firstDelayMs = 0) { ITickClass::startTick(firstDelayMs); };
        void stopTick() { ITickClass::stopTick(); };
    protected:
        void onTick(int64_t now) override { mTickCB(now); };
    };

protected:
    TickMgr();
    virtual ~TickMgr();

public:
    void   init();
    size_t size() const;
    void   clear();

    bool   addTick(ITickClass* listener, int64_t intervalMs, int64_t firstDelayMs = -1);
    bool   removeTick(ITickClass* listener);
    bool   hasTick(const ITickClass* listener) const;

    bool   runTickNow(ITickClass* listener);
    bool   updateInterval(ITickClass* listener, int64_t intervalMs, bool restartFromNow = true);

protected:
    virtual int checkEvents() override;
    virtual int handleEvents() override;

private:
    struct TickRecord {
        ITickClass*  listener{ nullptr };    // 回调对象
        bool         cancelled{ false };     // 取消标识
        int64_t      intervalMs{ 0 };        // Tick 周期
        int64_t      nextFireMs{ 0 };        // 下次触发时间
    };

private:
    static const size_t kInvalidIndex = static_cast<size_t>(-1);

private:
    bool shouldRunBefore(const TickRecord& lhs, const TickRecord& rhs) const;
    bool isDispatching() const;

    TickRecord* peekFront() const;
    TickRecord* findDispatchingRecord(const ITickClass* listener) const;
    size_t      findScheduledIndex(const ITickClass* listener) const;
    bool        isRegistered(const ITickClass* listener) const;

    TickRecord* acquireRecord();
    void        recycleRecord(TickRecord* record);
    void        destroyRecord(TickRecord* record);
    void        destroyRecordList(std::vector<TickRecord*>& records);

    void        insertOrdered(TickRecord* record);
    TickRecord* extractScheduledRecord(const ITickClass* listener);
    TickRecord* extractFrontIfDue(int64_t nowMs);

    bool        beginDispatch(TickRecord* record, int64_t nowMs);
    void        finishDispatch();
    bool        dispatchRecord(TickRecord* record, int64_t nowMs);

private:
    TickRecord*              mDispatchingRecord{ nullptr }; // 当前正在执行 onTick 的任务，脱离 mListeners
    std::vector<TickRecord*> mListeners;                    // 已注册任务，按 nextFireMs 从小到大有序
    std::vector<TickRecord*> mRecordPool;                   // TickRecord 缓存池
};

#endif // __TICK_MGR_H__
