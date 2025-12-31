/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:49
 * @LastEditTime: 2025-12-31 18:31:48
 * @FilePath: /kk_frame/src/app/managers/timer_mgr.cc
 * @Description: 定时器管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#include "timer_mgr.h"

#include <core/systemclock.h>
#include <cdlog.h>

 /// @brief 构造
TimerMgr::TimerMgr() {
    mInited = false;
    mTimerId = 0;
    mIdOverflow = false;
}

/// @brief 析构
TimerMgr::~TimerMgr() {
    mFreed.clear();
    mWorked.clear();
    mWorking.clear();

    for (TimerData* newDat : mBlocks) { free(newDat); }
    mBlocks.clear();
}

/// @brief 初始化
void TimerMgr::init() {
#if 1 // 获取当前线程的Looper
    cdroid::Looper::getForThread()->addEventHandler(this);
#else
    cdroid::Looper::getMainLooper()->addEventHandler(this);
#endif
    mInited = true;
}

/// @brief 启动定时器
/// @param timespace 时间间隔
/// @param timercb 回调对象
/// @param param 回调参数(可为数值可为指针)
/// @param repeat 重复次数(0为无限次)
/// @return 定时器ID
uint32_t TimerMgr::addTimer(uint32_t timespace, ITimer* timercb, size_t param, uint32_t repeat) {
    if (!mInited) return 0;
    if (!timercb) return 0;
    if (timespace < 10) timespace = 10;

    TimerData* timer = newTimer();
    if (!timer) {
        LOGE("New timer fail. space=%d", timespace);
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

    LOGD("Id=%u next=%ld count=%d", timer->id, timer->nextTime, mWorking.size());

    return timer->id;
}

/// @brief 根据ID删除定时器
/// @param id 定时器ID
/// @return 删除成功返回true
bool TimerMgr::delTimer(uint32_t id) {
    auto it = mWorking.find(id);
    if (it == mWorking.end()) return false;
    it->second->delit = true;
    freeTimer(it->second);
    return true;
}

/// @brief 根据回调删除定时器
/// @param timercb 定时器回调
/// @return 删除的定时器数量
int TimerMgr::delTimer(ITimer* timercb) {
    int count;
    if (!timercb) return 0;
    count = 0;
    for (auto it = mWorking.begin(); it != mWorking.end();) {
        if (it->second->sink == timercb) {
            auto itDel = it++;
            count++;
            freeTimer(itDel->second);
        } else {
            it++;
        }
    }
    if (count > 0) LOGD("Free timer. timercb=%p count=%d", timercb, count);
    return count;
}

/// @brief 新建定时器实例
/// @return 定时器指针
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

/// @brief 释放定时器
/// @param dat 定时器实例
void TimerMgr::freeTimer(TimerData* dat) {
    auto it = mWorking.find(dat->id);
    if (it == mWorking.end()) return;
    LOGD("Id=%u", dat->id);
    dat->delit = true;
    mWorked.remove(dat);
    mFreed.push_back(dat);
    mWorking.erase(it);
}

/// @brief 排序定时器
/// @param dat 定时器实例
void TimerMgr::addWorked(TimerData* dat) {
    dat->nextTime = dat->begTime + (int64_t)dat->timespace * (dat->count + 1);

    auto it = mWorked.begin(), itlast = it;
    for (; it != mWorked.end(); it++) {
        if (dat->nextTime >= (*it)->nextTime) { itlast = it; }
    }

    itlast++;
    if (itlast != mWorked.end()) {
        mWorked.insert(itlast, dat);
    } else {
        mWorked.push_back(dat);
    }

    LOGV("Id=%u nextTime=%ld count=%d", dat->id, dat->nextTime, mWorked.size());
}

/// @brief 打印工作中定时器
void TimerMgr::dumpWork() {
    for (auto it = mWorked.begin(); it != mWorked.end(); it++) {
        TimerData* dat = *it;
        LOGD("Id=%d time=%ld", dat->id, dat->nextTime);
    }
}

/// @brief 检查首个定时器是否已达时间
/// @return
int TimerMgr::checkEvents() {
    if (mWorked.empty() || !mInited) return 0;
    int64_t    nowms = cdroid::SystemClock::uptimeMillis();
    TimerData* headTimer = mWorked.front();
    if (headTimer->nextTime <= nowms) return 1;
    return 0;
}

/// @brief 处理定时器任务
/// @return 
int TimerMgr::handleEvents() {
    int     diffms, eventCount = 0;
    int64_t nowms = cdroid::SystemClock::uptimeMillis();

    do {
        TimerData* headTimer = mWorked.front();
        if (headTimer->nextTime > nowms) { break; }

        headTimer->count++;
        headTimer->sink->onTimer(headTimer->id, headTimer->param, headTimer->count);

        if (!headTimer->delit && (headTimer->repeat == 0 || headTimer->count > headTimer->repeat)) {
            mWorked.pop_front();
            addWorked(headTimer);
        } else {
            freeTimer(headTimer);
        }

        eventCount++;

    } while (eventCount < 5 && !mWorked.empty());

    if ((diffms = cdroid::SystemClock::uptimeMillis() - nowms) > 100) {
        LOGW("Handle timer events more than 100ms. use=%dms", diffms);
    }

    return eventCount;
}