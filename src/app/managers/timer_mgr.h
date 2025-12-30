/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:44
 * @LastEditTime: 2025-12-30 11:28:29
 * @FilePath: /kk_frame/src/app/managers/timer_mgr.h
 * @Description: 定时器管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TIMER_MGR_H__
#define __TIMER_MGR_H__

#include "template/singleton.h"
#include <core/looper.h>
#include <map>

#define g_timer TimerMgr::instance()

/// @brief 定时器管理类
class TimerMgr :public Singleton<TimerMgr>,
    public cdroid::EventHandler {
    friend class Singleton<TimerMgr>;
public:
    typedef enum {
        TIMER_WORKING = 0,    // 工作计时器

        DEFAULT_TIMER_MAX,    // 默认定时器最大值
    } DEFAULT_TIMER_TYPE;

    class WorkTimer {
    public:
        WorkTimer() { };
        virtual ~WorkTimer() { };
        virtual void onTimer(uint32_t id, size_t param, uint32_t count) = 0;
    };
private:
#pragma pack(1)
    struct TimerData {
        uint32_t   id;         // ID编号
        uint8_t    delit;      // 是否删除
        uint32_t   timespace;  // 时间间隔
        WorkTimer* sink;       // 回调对象
        size_t     param;      // 参数
        int16_t    repeat;     // 重复次数
        uint32_t   count;      // 计时次数
        int64_t    begTime;    // 开始时间
        int64_t    nextTime;   // 下次时间
    };
#pragma pack()

    uint32_t                        mTimerId;    // 计时器ID
    bool                            mIdOverflow; // 计时器ID是否溢出
    std::vector<TimerData*>         mBlocks;     // 计时器数据块
    std::list<TimerData*>           mFreed;      // 空闲计时器
    std::list<TimerData*>           mWorked;     // 已排序的待处理计时器列表
    std::map<uint32_t, TimerData*>  mWorking;    // 当前活跃的计时器映射

protected:
    std::map<
        uint8_t, std::pair<uint32_t, WorkTimer*>
    > mTimerIdMap;   // 记录默认Timer与运行中Timer的映射关系

    std::unordered_map<
        size_t, std::function<WorkTimer* ()>
    > mDefaultTimerFactory; // 默认Timer工厂

private:
    TimerMgr();

public:
    ~TimerMgr();
    void init();

    // 内置计时任务
    void start(DEFAULT_TIMER_TYPE param, uint32_t timespace, int16_t repeat = -1);
    void stop(DEFAULT_TIMER_TYPE param);

    // 自定义计时任务
    uint32_t             addTimer(uint32_t timespace, WorkTimer* timercb, size_t param, int16_t repeat = -1);
    bool                 delTimer(uint32_t id);
    int                  delTimer(WorkTimer* timercb);
    int                  delTimerFromParam(size_t param);

protected:
    TimerData* newTimer();
    void       freeTimer(TimerData* dat);
    void       addWorked(TimerData* dat);
    void       dumpWork();

    virtual int checkEvents() override;
    virtual int handleEvents() override;
};

#endif // __TIMER_MGR_H__