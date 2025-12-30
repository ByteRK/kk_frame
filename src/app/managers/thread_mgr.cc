 /*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-04-30 11:12:00
 * @LastEditTime: 2025-12-30 11:29:59
 * @FilePath: /kk_frame/src/app/managers/thread_mgr.cc
 * @Description: 公用类
 * @BugList: 
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
 */


#include "thread_mgr.h"
#include "timer_mgr.h"

#include <core/app.h>
#include <cdlog.h>

ThreadPool::ThreadPool() {
    mIsInit = false;
    mTaskId = 0;
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
    for (int i = 0; i < count; i++) {
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

int ThreadPool::add(ThreadTask *sink, void *data,bool isRecycle) {
    if (!sink) return -1;

    TaskData *td = new TaskData();
    td->id       = ++mTaskId;
    td->isDel    = 0;
    td->atime    = SystemClock::uptimeMillis();
    td->sink     = sink;
    td->data     = data;
    td->isRecycle  = isRecycle;
    mWaitTasks.push_back(td);

    LOGV("task add. id=%d", td->id);

    return td->id;
}

int ThreadPool::del(int taskId) {
    for (ThreadData *td : mThreads) {
        if (td->tdata->id == taskId) {
            td->tdata->isDel = 1;
            return 0;
        }
    }
    for (TaskData *td : mWaitTasks) {
        if (td->id == taskId) {
            td->isDel = 1;
            return 0;
        }
    }

    return -1;
}

int ThreadPool::checkEvents() {
    return mWaitTasks.size() + mMainTasks.size();
}

int ThreadPool::handleEvents() {
    // 处理执行完成的任务

    while (mWaitTasks.size()) {
        ThreadData *th = getIdle();
        if (!th) break;

        TaskData *td = mWaitTasks.front();
        th->tdata    = td;
        th->status   = TS_BUSY;
        LOG(VERBOSE) << "add thread. id=" << td->id << " tid=" << th->tid;

        mWaitTasks.pop_front();
    }

    int64_t startTime, diffTime;
    startTime = SystemClock::uptimeMillis();
    mMutex.lock();
    for (int i = 0; i < 3 && mMainTasks.size(); i++) {
        TaskData *td = mMainTasks.front();
        LOGV("task end. id=%d del=%d time=%dms", td->id, (int)td->isDel, int(startTime - td->atime));
        if (td->isDel == 0) td->sink->onMain(td->data);
        mMainTasks.pop_front();
        if(td->isRecycle) delete td->sink;
        delete td;

        diffTime = SystemClock::uptimeMillis() - startTime;
        if (diffTime > 500) {
            LOGW("Task on main more time. %dms", (int)diffTime);
            break;
        }
    }
    mMutex.unlock();

    return 1;
}

void ThreadPool::onThread(ThreadData *thData) {
    int ret,idleCount;
    thData->tid    = std::this_thread::get_id();
    thData->status = TS_IDLE;
    thData->itime  = SystemClock::uptimeMillis();

    LOG(VERBOSE) << "new thread. id=" << thData->tid;

    idleCount = 0;
    while (thData->status != TS_OVER) {
        if (thData->status == TS_BUSY) {
            LOGV("on task beg. id=%d", thData->tdata->id);
            if ((thData->tdata->isDel && (ret = 10000)) || (ret = thData->tdata->sink->onTask(thData->tdata->data)) == 0) {
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
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }            
        }
    }

    delete thData;
}

ThreadPool::ThreadData *ThreadPool::getIdle() {
    for (ThreadData *td : mThreads) {
        if (td->status == TS_IDLE) { return td; }
    }
    return 0;
}

