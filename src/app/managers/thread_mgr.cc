/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-02-03 10:22:19
 * @FilePath: /kk_frame/src/app/managers/thread_mgr.cc
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "thread_mgr.h"
#include "timer_mgr.h"

#include <core/app.h>
#include <cdlog.h>
#include <inttypes.h>

#define THREAD_RECYCLE_TIME 60000 // 线程回收时间

ThreadPool::ThreadPool() {
    mIsInit     = false;
    mTaskId     = 0;
    mHandleTime = 0;
    mCount      = 0;
    App::getInstance().addEventHandler(this);
}

ThreadPool::~ThreadPool() {
    App::getInstance().removeEventHandler(this);

    for (ThreadData *td : mThreads) { td->status = TS_OVER; }
    mThreads.clear();
}

int ThreadPool::init(int count) {
    if (mIsInit) return 0;
    mIsInit = true;
    if (count <= 0) { count = 1; }
    mCount = count;
    return 0;
}

int ThreadPool::add(ThreadTask *sink, void *data) {
    if (!sink) return -1;

    TaskData *td = new TaskData();
    td->id       = ++mTaskId;
    td->isDel    = 0;
    td->atime    = SystemClock::uptimeMillis();
    td->sink     = sink;
    td->data     = data;
    mWaitTasks.push_back(td);

    LOGV("task add. id=%d", td->id);

    return td->id;
}

int ThreadPool::del(int taskId) {
    // 已经进入线程
    for (ThreadData *td : mThreads) {
        if (td->tdata->id == taskId) {
            LOGV("del thread task. id=%d", td->tdata->id);
            td->tdata->isDel = 1;
            return 0;
        }
    }

    // 未进入线程
    for (auto it = mWaitTasks.begin(); it != mWaitTasks.end(); it++) {
        TaskData *td = (*it);
        if (td->id == taskId) {
            LOGV("del wait task. id=%d", td->id);
            mWaitTasks.erase(it);
            delete td;
            return 0;
        }
    }

    return -1;
}

int ThreadPool::checkEvents() {
    size_t n = mWaitTasks.size() + mMainTasks.size();
    if (n > 0) return 1;
    // 保证至少1s处理一次
    int64_t nowTick = SystemClock::uptimeMillis();
    if (nowTick - mHandleTime > 1000) { return 1; }
    return 0;
}

int ThreadPool::handleEvents() {
    // 处理执行完成的任务
    int  i;
    bool noTask;

    mHandleTime = SystemClock::uptimeMillis();

    noTask = mWaitTasks.empty();
    while (mWaitTasks.size()) {
        ThreadData *th = getIdle();
        if (!th) break;

        TaskData *td = mWaitTasks.front();
        th->tdata    = td;
        th->status   = TS_BUSY;
        LOGV("add thread. id=%d tid=%lu", td->id, th->tid);

        mWaitTasks.pop_front();
    }

    int64_t startTime, diffTime;
    startTime = SystemClock::uptimeMillis();
    mMutex.lock();
    for (i = 0; i < 3 && mMainTasks.size(); i++) {
        TaskData *td = mMainTasks.front();
        mMainTasks.pop_front();

        LOGV("task main  in. id=%d del=%d time=%" PRId64, td->id, (int)td->isDel, startTime - td->atime);
        if (td->isDel == 0) td->sink->onMain(td->id, td->data);
        LOGV("task main out. id=%d", td->id);

        delete td;

        diffTime = SystemClock::uptimeMillis() - startTime;
        if (diffTime > 500) {
            LOGW("Task on main more time. %dms", (int)diffTime);
            break;
        }
    }
    mMutex.unlock();
    if (i > 0) LOGV("task main end. i=%d", i);

    // 回收空闲线程
    if (noTask) { freeThread(); }

    return 1;
}

void ThreadPool::onThread(ThreadData *thData) {
    int ret, idleCount;
    thData->tid    = std::this_thread::get_id();
    thData->status = TS_IDLE;
    thData->itime  = SystemClock::uptimeMillis();

    LOGI("new thread. id=%lu", thData->tid);

    idleCount = 0;
    while (thData->status != TS_OVER) {
        if (thData->status == TS_BUSY) {
            LOGV("on task beg. id=%d", thData->tdata->id);
            if ((thData->tdata->isDel && (ret = 10000)) ||
                (ret = thData->tdata->sink->onTask(thData->tdata->id, thData->tdata->data)) == 0) {
                mMutex.lock();
                mMainTasks.push_back(thData->tdata);
                mMutex.unlock();
                thData->itime  = SystemClock::uptimeMillis();
                thData->status = TS_IDLE;
            }
            LOGV("on task end. id=%d ret=%d", thData->tdata->id, ret);
        } else {
            idleCount++;
            if (idleCount >= 10) {
                idleCount = 0;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    if (thData->th) { delete thData->th; thData->th = 0; };
    if (thData) { delete thData; thData = 0; };
}

ThreadPool::ThreadData *ThreadPool::getIdle() {
    for (ThreadData *td : mThreads) {
        if (td->status == TS_IDLE) { return td; }
    }
    if (mThreads.size() < mCount) {
        ThreadData *thData = new ThreadData();
        thData->status     = TS_NULL;
        thData->itime      = 0;
        thData->tdata      = 0;
        thData->th         = new std::thread(std::bind(&ThreadPool::onThread, this, std::placeholders::_1), thData);
        thData->th->detach();
        mThreads.push_back(thData);
    }
    return 0;
}

void ThreadPool::freeThread() {
    if (mThreads.empty() || THREAD_RECYCLE_TIME <= 0) { return; }

    int64_t nowTick = SystemClock::uptimeMillis();

    // 回收空闲时间过长的线程
    for (auto it = mThreads.begin(); it != mThreads.end();) {
        ThreadData *thData = *it;
        if (thData->status != TS_IDLE) {
            it++;
            continue;
        }
        if (thData->itime <= 0) {
            it++;
            continue;
        }
        if (nowTick - thData->itime >= THREAD_RECYCLE_TIME) {
            LOG(INFO) << "free thread. id=" << thData->tid;
            thData->status = TS_OVER;
            it             = mThreads.erase(it);
        } else {
            it++;
        }
    }
}