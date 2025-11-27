/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:14:15
 * @LastEditTime: 2025-09-01 16:55:57
 * @FilePath: /kk_frame/src/common/comm_class.h
 * @Description: 公用类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/


#ifndef __comm_class_h__
#define __comm_class_h__

#include "comm_func.h"

#include <thread>
#include <list>
#include <view/view.h>

// 线程任务
class ThreadTask {
public:
    virtual int  onTask(void* data) = 0; // 执行任务 0-完成 !0-未完成
    virtual void onMain(void* data) = 0; // 任务执行完成，回到主线程
};

// 线程池
class ThreadPool : public EventHandler {
protected:
    typedef enum {
        TS_NULL = 0,
        TS_IDLE,
        TS_BUSY,
        TS_OVER,
    }ThreadStatus;
    typedef struct {
        int         id;
        uint8_t       isDel;
        int64_t     atime;
        ThreadTask* sink;
        void* data;
        bool        isRecycle;
    }TaskData;
    typedef struct {
        std::thread::id       tid;
        int       status;
        int64_t   itime;
        std::thread* th;
        TaskData* tdata;
    }ThreadData;
public:
    static ThreadPool* ins();
    int init(int count);
    int add(ThreadTask* sink, void* data, bool isRecycle = false);
    int del(int taskId);
protected:
    ThreadPool();
    ~ThreadPool();
    virtual int checkEvents();
    virtual int handleEvents();

    void onThread(ThreadData* thData);
    ThreadData* getIdle();
private:
    bool                    mIsInit;
    int                     mTaskId;
    std::mutex              mMutex;
    std::list<ThreadData*> mThreads;
    std::list<TaskData*>   mWaitTasks; // 等待被执行的任务    
    std::list<TaskData*>   mMainTasks; // 执行完成的任务
};

// 定时器
class ITimer {
public:
    virtual ~ITimer() = default;
    virtual void onTimer(uint32_t id, size_t param, uint32_t count) = 0;
};
class TimerManager : public EventHandler {
protected:
#pragma pack(1)
    struct TimerData {
        uint32_t id;
        uint8_t  delit;
        uint32_t timespace;
        ITimer* sink;
        size_t   param;
        int16_t  repeat;
        uint32_t count;
        int64_t  begTime;
        int64_t  nextTime;
    };
#pragma pack()
public:
    static TimerManager* ins();
    uint32_t             addTimer(uint32_t timespace, ITimer* timercb, size_t param = 0, int16_t repeat = -1);
    bool                 delTimer(uint32_t id);
    int                  delTimer(ITimer* timercb);
    int                  delTimerFromParam(size_t param);

protected:
    TimerManager();
    ~TimerManager();
    TimerData* newTimer();
    void       freeTimer(TimerData* dat);
    void       addWorked(TimerData* dat);
    void       dumpWork();

    virtual int checkEvents();
    virtual int handleEvents();

private:
    static TimerManager* sTimerIns;
    uint32_t                        mTimerId;
    bool                            mIdOverflow;
    std::vector<TimerData*>        mBlocks;
    std::list<TimerData*>          mFreed;
    std::list<TimerData*>          mWorked;
    std::map<uint32_t, TimerData*> mWorking;
};


#endif
