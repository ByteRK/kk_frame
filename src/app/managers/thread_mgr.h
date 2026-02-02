/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:14:15
 * @LastEditTime: 2026-02-02 17:05:51
 * @FilePath: /kk_frame/src/app/managers/thread_mgr.h
 * @Description: 线程池管理
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/


#ifndef __THREAD_MGR_H__
#define __THREAD_MGR_H__

#include "template/singleton.h"
#include <view/view.h>
#include <thread>
#include <stdint.h>

#define g_threadMgr ThreadPool::instance()

// 线程任务
class ThreadTask {
public:
    virtual int  onTask(int id, void *data) = 0; // 执行任务 0-完成 !0-未完成
    virtual void onMain(int id, void *data) = 0; // 任务执行完成，回到主线程
};

// 线程池
class ThreadPool : public Singleton<ThreadPool>,
    public EventHandler {
    friend Singleton<ThreadPool>;
protected:
    typedef enum {
        TS_NULL = 0,
        TS_IDLE,
        TS_BUSY,
        TS_OVER,
    } ThreadStatus;
    typedef struct {
        int         id;
        uint8_t     isDel;
        int64_t     atime;
        ThreadTask *sink;
        void       *data;
    } TaskData;
    typedef struct {
        std::thread::id tid;
        int             status;
        int64_t         itime;
        std::thread    *th;
        TaskData       *tdata;
    } ThreadData;

public:
    static ThreadPool *ins();
    int                init(int count);
    int                add(ThreadTask *sink, void *data);
    int                del(int taskId);

protected:
    ThreadPool();
    ~ThreadPool();
    int checkEvents() override;
    int handleEvents() override;

    void        onThread(ThreadData *thData);
    void        freeThread();
    ThreadData *getIdle();

private:
    bool                    mIsInit;
    int                     mCount;
    int                     mTaskId;
    int64_t                 mHandleTime;
    std::mutex              mMutex;
    std::list<ThreadData *> mThreads;
    std::list<TaskData *>   mWaitTasks; // 等待被执行的任务
    std::list<TaskData *>   mMainTasks; // 执行完成的任务
};

#endif // __THREAD_MGR_H__
