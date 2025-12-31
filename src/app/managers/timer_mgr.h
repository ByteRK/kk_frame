/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:44
 * @LastEditTime: 2025-12-31 18:12:41
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
    /// @brief 定时器回调基类
    class ITimer {
    public:
        virtual void onTimer(uint32_t id, size_t param, uint32_t count) = 0;
    };
protected:
#pragma pack(1)
    struct TimerData { // 定时器定义
        uint32_t id;         // ID编号
        uint8_t  delit;      // 是否删除
        uint32_t timespace;  // 时间间隔
        ITimer  *sink;       // 回调对象
        size_t   param;      // 参数(可为数值可为指针)
        uint32_t repeat;     // 重复次数
        uint32_t count;      // 计时次数
        int64_t  begTime;    // 开始时间
        int64_t  nextTime;   // 下次时间
    };
#pragma pack()
protected:
    TimerMgr();

public:
    ~TimerMgr();
    void                 init();
    uint32_t             addTimer(uint32_t timespace, ITimer *timercb, size_t param = 0, uint32_t repeat = 0);
    bool                 delTimer(uint32_t id);
    int                  delTimer(ITimer *timercb);

protected:
    TimerData *newTimer();
    void       freeTimer(TimerData *dat);
    void       addWorked(TimerData *dat);
    void       dumpWork();

    virtual int checkEvents() override;
    virtual int handleEvents() override;

private:
    bool                            mInited;       // 是否已初始化
    uint32_t                        mTimerId;      // ID计数器
    bool                            mIdOverflow;   // ID是否溢出
    std::vector<TimerData *>        mBlocks;       // 定时器实例列表
    std::list<TimerData *>          mFreed;        // 空闲定时器实例
    std::list<TimerData *>          mWorked;       // 工作中定时器列表(已按时间排序)
    std::map<uint32_t, TimerData *> mWorking;      // 工作中定时器映射
};

#endif // __TIMER_MGR_H__