 /*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-04-30 11:12:00
 * @LastEditTime: 2025-09-01 14:31:15
 * @FilePath: /kk_frame/src/common/comm_class.cc
 * @Description: 公用类
 * @BugList: 
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
 */


#include "comm_class.h"
#include <core/app.h>
#include "timer_mgr.h"
////////////////////////////////////////////////////////////////////////////////////////
ThreadPool *ThreadPool::ins() {
    static ThreadPool sIns;
    return &sIns;
}

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


/////////////////////////////////////////////////////////////////////////////////
TimerManager *TimerManager::sTimerIns = 0;

TimerManager::TimerManager() {
    mTimerId    = 0;
    mIdOverflow = false;
    Looper::getForThread()->addEventHandler(this);
}

TimerManager::~TimerManager() {
    mFreed.clear();
    mWorked.clear();
    mWorking.clear();

    for (TimerData *newDat : mBlocks) { free(newDat); }
    mBlocks.clear();
}

TimerManager *TimerManager::ins() {
    if (!sTimerIns) { sTimerIns = new TimerManager(); }
    return sTimerIns;
}

uint32_t TimerManager::addTimer(uint32_t timespace, ITimer *timercb, size_t param /* = 0 */,
                                int16_t repeat /* = -1 */) {
    if (!timercb) return 0;
    if (timespace < 10) timespace = 10;

    TimerData *timer = newTimer();
    if (!timer) {
        LOGE("new timer fail. space=%d", timespace);
        return 0;
    }

    timer->timespace = timespace;
    timer->sink      = timercb;
    timer->param     = param;
    timer->repeat    = repeat;
    timer->count     = 0;
    timer->begTime   = SystemClock::uptimeMillis();
    timer->delit     = false;

    mWorking.insert(std::make_pair(timer->id, timer));
    addWorked(timer);

    LOGD("[%p]dat=%u next=%ld count=%d timer->begTime = %lld", timer, timer->id, timer->nextTime, mWorking.size(),timer->begTime);

    return timer->id;
}

bool TimerManager::delTimer(uint32_t id) {
    auto it = mWorking.find(id);
    if (it == mWorking.end()) return false;
    it->second->delit = true;
    freeTimer(it->second);
    return true;
}

int TimerManager::delTimer(ITimer *timercb) {
    int count = 0;
    if (!timercb) return count;
free_timer:
    for (auto it = mWorking.begin(); it != mWorking.end();) {
        if (it->second->sink == timercb) {
            count++;
            freeTimer(it->second);
            goto free_timer;
        } else {
            it++;
        }
    }
    if (count > 0) LOGI("Free timer. timercb=%p count=%d", timercb, count);
    return count;
}

int TimerManager::delTimerFromParam(size_t param){
    int count = 0;
free_timer:
    for (auto it = mWorking.begin(); it != mWorking.end();) {
        if (it->second->param == param) {
            count++;
            freeTimer(it->second);
            goto free_timer;
        } else {
            it++;
        }
    }
    if (count > 0) LOGV("Free timer. param=%p count=%d", param, count);
    return count;
}

TimerManager::TimerData *TimerManager::newTimer() {
    TimerData *dat;
    if (mFreed.empty()) {
        const int  blockCount = 10;
        TimerData *newDat     = (TimerData *)calloc(1, blockCount * sizeof(TimerData));
        mBlocks.push_back(newDat);
        for (int i = 0; i < blockCount; i++) {
            mFreed.push_back(newDat);
            newDat++;
        }
    }
    mTimerId++;
    if (mTimerId == 0 || mIdOverflow) {
        if (!mIdOverflow) mIdOverflow = true;
        if (mTimerId == 0) mTimerId++;
        do {
            if (mWorking.find(mTimerId) == mWorking.end()) break;
            mTimerId++;
        } while (true);
    }
    dat     = mFreed.front();
    dat->id = mTimerId;
    mFreed.pop_front();
    return dat;
}

void TimerManager::freeTimer(TimerManager::TimerData *dat) {
    auto it = mWorking.find(dat->id);
    if (it == mWorking.end()) return;
    LOGV("[%p]dat=%u", dat, dat->id);
    dat->delit = true;
    mWorked.remove(dat);
    mFreed.push_back(dat);
    mWorking.erase(it);
}

void TimerManager::addWorked(TimerManager::TimerData *dat) {
    dat->nextTime = dat->begTime + (int64_t)dat->timespace * (dat->count + 1);

    for (auto it = mWorked.begin(); it != mWorked.end(); it++) {
        if (dat->nextTime < (*it)->nextTime) { 
            mWorked.insert(it, dat);
            return;
        }
    }
    mWorked.push_back(dat);

    LOGV("add work. dat=%u nextTime=%ld count=%d", dat->id, dat->nextTime, mWorked.size());
}

int TimerManager::checkEvents() {
    if (mWorked.empty()) return 0;
    int64_t    nowms     = SystemClock::uptimeMillis();
    TimerData *headTimer = mWorked.front();
    if (headTimer->nextTime <= nowms) return 1;
    return 0;
}

int TimerManager::handleEvents() {
    int     diffms, eventCount = 0;
    int64_t nowms = SystemClock::uptimeMillis();

    do {
        TimerData *headTimer = mWorked.front();
        if (headTimer->nextTime > nowms) { break; }

        headTimer->count++;
        headTimer->sink->onTimer(headTimer->id, headTimer->param, headTimer->count);

        if (!headTimer->delit && (headTimer->repeat == -1 || headTimer->count < headTimer->repeat)) {
            mWorked.pop_front();
            addWorked(headTimer);
        } else {
            freeTimer(headTimer);
        }

        eventCount++;

    } while (eventCount < 5 && !mWorked.empty());

    if ((diffms = SystemClock::uptimeMillis() - nowms) > 100) {
        LOGW("handle timer events more than 100ms. use=%dms", diffms);
    }

    return eventCount;
}

void TimerManager::dumpWork() {
    for (auto it = mWorked.begin(); it != mWorked.end(); it++) {
        TimerData *dat = *it;
        LOGD("[%p]id=%d time=%ld", dat, dat->id, dat->nextTime);
    }
}
