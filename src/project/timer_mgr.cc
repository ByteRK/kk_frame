/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:49
 * @LastEditTime: 2025-09-02 20:26:50
 * @FilePath: /kk_frame/src/project/timer_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#include "timer_mgr.h"
#include "work_timer.h"
#include <core/systemclock.h>

TimerMgr::TimerMgr() {
    mTimerId = 0;
    mIdOverflow = false;
    mTimerIdMap.clear();
}

TimerMgr::~TimerMgr() {
    mFreed.clear();
    mWorked.clear();
    mWorking.clear();

    for (TimerData* newDat : mBlocks) { free(newDat); }
    mBlocks.clear();
}

void TimerMgr::init() {
    cdroid::Looper::getForThread()->addEventHandler(this);
    mDefaultTimerFactory[TIMER_WORKING] = []() { return new CookingTimer(); };
}

/// @brief 启动内置定时器
/// @param param  定时器类型
/// @param timespace  定时器间隔
/// @param repeat  定时次数
void TimerMgr::start(size_t param, uint32_t timespace, int16_t repeat) {
    // 判断是否已存在该类型的定时器
    auto it = mTimerIdMap.find(param);
    if (it == mTimerIdMap.end()) {
        // 创建该类型的定时器WorkTimer
        auto timerIt = mDefaultTimerFactory.find(param);
        if (timerIt != mDefaultTimerFactory.end()) {
            WorkTimer* tBase = timerIt->second(); // 调用构造函数创建页面
            uint32_t id = addTimer(timespace, tBase, param, repeat); // 创建定时器
            mTimerIdMap[param] = { id, tBase };
            LOGE("TimerMgr start id:%d param:%d", id, param);
        } else {
            throw std::runtime_error("TimerMgr::start mDefaultTimerFactory id not find  param !!!!!");
        }
    } else {
        LOGE("timer is running");
    }
}

/// @brief 停止内置定时器
/// @param param  定时器类型
/// @note 回调类将被析构
void TimerMgr::stop(size_t param) {
    // 找到该类型的定时器
    auto it = mTimerIdMap.find(param);
    if (it != mTimerIdMap.end()) {
        uint32_t id = it->second.first;
        delTimer(id);  // 取消定时器
        delete it->second.second;
        mTimerIdMap.erase(it);
        LOGE("TimerMgr stop id:%d param:%d", id, param);
    } else {
        LOGE("timer is not running");
    }
}

/// @brief 增加默认计时器
/// @param timespace 
/// @param timercb 
/// @param param 
/// @param repeat 
/// @return 
uint32_t TimerMgr::addTimer(uint32_t timespace, WorkTimer* timercb, size_t param, int16_t repeat) {
    if (!timercb) return 0;
    if (timespace < 10) timespace = 10;

    TimerData* timer = newTimer();
    if (!timer) {
        LOGE("new timer fail. space=%d", timespace);
        return 0;
    }

    timer->timespace = timespace;
    timer->sink = timercb;
    timer->param = param;
    timer->repeat = repeat;
    timer->count = 0;
    timer->begTime = cdroid::SystemClock::uptimeMillis();
    timer->delit = false;

    mWorking.insert(std::make_pair(timer->id, timer));
    addWorked(timer);

    LOGD("[%p]dat=%u next=%ld count=%d timer->begTime = %lld", timer, timer->id, timer->nextTime, mWorking.size(), timer->begTime);

    return timer->id;
}

/// @brief 根据ID删除计时器
/// @param id 
/// @return 
bool TimerMgr::delTimer(uint32_t id) {
    auto it = mWorking.find(id);
    if (it == mWorking.end()) return false;
    it->second->delit = true;
    freeTimer(it->second);
    return true;
}

/// @brief 根据ID删除计时器
/// @param id 
/// @return 
int TimerMgr::delTimer(WorkTimer* timercb) {
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

/// @brief 根据参数删除计时器
/// @param param 
/// @return 
int TimerMgr::delTimerFromParam(size_t param) {
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

/// @brief 新建计时器
/// @return 
TimerMgr::TimerData* TimerMgr::newTimer() {
    TimerData* dat;
    if (mFreed.empty()) {
        const int  blockCount = 10;
        TimerData* newDat = (TimerData*)calloc(1, blockCount * sizeof(TimerData));
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
    dat = mFreed.front();
    dat->id = mTimerId;
    mFreed.pop_front();
    return dat;
}

/// @brief 释放计时器
/// @param dat 
void TimerMgr::freeTimer(TimerData* dat) {
    auto it = mWorking.find(dat->id);
    if (it == mWorking.end()) return;
    LOGV("[%p]dat=%u", dat, dat->id);
    dat->delit = true;
    mWorked.remove(dat);
    mFreed.push_back(dat);
    mWorking.erase(it);
}

/// @brief 增加计时器
/// @param dat 
void TimerMgr::addWorked(TimerData* dat) {
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

/// @brief 打印计时器
void TimerMgr::dumpWork() {
    for (auto it = mWorked.begin(); it != mWorked.end(); it++) {
        TimerData* dat = *it;
        LOGD("[%p]id=%d time=%ld", dat, dat->id, dat->nextTime);
    }
}

/// @brief 
/// @return 
int TimerMgr::checkEvents() {
    if (mWorked.empty()) return 0;
    int64_t    nowms = cdroid::SystemClock::uptimeMillis();
    TimerData* headTimer = mWorked.front();
    if (headTimer->nextTime <= nowms) return 1;
    return 0;
}

/// @brief 
/// @return 
int TimerMgr::handleEvents() {
    int     diffms, eventCount = 0;
    int64_t nowms = cdroid::SystemClock::uptimeMillis();

    do {
        TimerData* headTimer = mWorked.front();
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

    if ((diffms = cdroid::SystemClock::uptimeMillis() - nowms) > 100) {
        LOGW("handle timer events more than 100ms. use=%dms", diffms);
    }

    return eventCount;
}