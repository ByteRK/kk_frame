/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:08:49
 * @LastEditTime: 2025-09-01 16:50:13
 * @FilePath: /kk_frame/src/project/work_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#include "work_mgr.h"
#include "global_data.h"
#include "base.h"
#include "proto.h"
#include "wind_mgr.h"

TimerMgr::TimerMgr() {

    mTimerId = 0;
    mIdOverflow = false;
    Looper::getForThread()->addEventHandler(this);

    mTimerIdMap.clear();

    mDefaultTimerFactory[TIMER_WORKING] = []() { return new CookingTimer(); };
}

TimerMgr::~TimerMgr() {
    mFreed.clear();
    mWorked.clear();
    mWorking.clear();

    for (TimerData* newDat : mBlocks) { free(newDat); }
    mBlocks.clear();
}

/// @brief 启动定时器
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

/// @brief 停止定时器
/// @param param  定时器类型
void TimerMgr::stop(size_t param) {
    // 找到该类型的定时器
    auto it = mTimerIdMap.find(param);
    if (it != mTimerIdMap.end()) {
        uint32_t id = it->second.first;
        delTimer(id);  // 取消定时器
        // delete it->second.timer;
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
    timer->begTime = SystemClock::uptimeMillis();
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

/// @brief 删除计时器
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
    int64_t    nowms = SystemClock::uptimeMillis();
    TimerData* headTimer = mWorked.front();
    if (headTimer->nextTime <= nowms) return 1;
    return 0;
}

/// @brief 
/// @return 
int TimerMgr::handleEvents() {
    int     diffms, eventCount = 0;
    int64_t nowms = SystemClock::uptimeMillis();

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

    if ((diffms = SystemClock::uptimeMillis() - nowms) > 100) {
        LOGW("handle timer events more than 100ms. use=%dms", diffms);
    }

    return eventCount;
}

/// @brief 烹饪定时器的构造函数
CookingTimer::CookingTimer() {
    // g_data->setLamp(true);
}

/// @brief 烹饪定时器的析构函数
CookingTimer::~CookingTimer() {
    // g_data->setLamp(true);
}

/// @brief 烹饪定时器
/// @param id 定时器id
/// @param param 定时器类型
/// @param count 定时次数
void CookingTimer::onTimer(uint32_t id, size_t param, uint32_t count) {
    //     LOGV("CookingTimer::onTimer id:%d param:%d count:%d", id, param, count);
    //     if (g_data->mStatus == ES_WORKING) {
    //         // 因为该 timer 已有时间修正的作用，因此可以直接 - 1 秒
    // #ifdef CDROID_X64
    //         g_data->mOverTime -= 10;
    // #else
    //         g_data->mOverTime--;
    // #endif
    //         if (g_data->mDoor) {
    //             if (g_window->getPageType() != PAGE_COOKING) {
    //                 g_windMgr->goTo(PAGE_COOKING);
    //             } else {
    //                 // 通知工作页面更新
    //                 g_windMgr->sendMsg(PAGE_COOKING, MSG_DOOR_CHANGE);
    //             }
    //         }

    //         // 当前步骤烹饪已结束
    //         if (g_data->mOverTime <= 0) {
    //             // 有下一步
    //             if (g_data->mRunStepIndex < g_data->mRunningModeData.modes.size() - 1) {
    //                 // 自动下一步
    //                 if (g_data->mRunningModeData.modes[g_data->mRunStepIndex].autoNext) {
    //                     g_data->mRunStepIndex++;
    //                     g_data->mAllTime = g_data->mRunningModeData.getStepWorkTime(g_data->mRunStepIndex);
    //                     g_data->mOverTime = g_data->mAllTime;

    //                 } else {
    //                     // 需弹窗提示
    //                     g_data->mStatus = ES_PAUSE;
    //                     g_data->mRunStepIndex++;
    //                     g_data->mAllTime = g_data->mRunningModeData.getStepWorkTime(g_data->mRunStepIndex);
    //                     g_data->mOverTime = g_data->mAllTime + 1;
    //                     g_TimerMgr->stop(TIMER_WORKING); // 停止工作的tick
    //                     PopNormal::BaseDataStr baseData;
    //                     baseData.title = "温馨提示";
    //                     baseData.type = PopNormal::PT_NEXT_STEP;
    //                     std::string nextInfo = g_data->mRunStepIndex == 1 ? "一" : "二";
    //                     baseData.info = "第" + nextInfo + "段烹饪已完成，\n请取出食材加工后再放入烹饪";
    //                     baseData.enterText = "下一步";
    //                     baseData.cancelText = "";
    //                     baseData.fromView = nullptr;
    //                     baseData.enterListener = [this]() {
    //                         g_data->mStatus = ES_WORKING;
    //                         g_TimerMgr->start(TIMER_WORKING, 1000);// 启动工作tick
    //                         g_windMgr->sendMsg(PAGE_COOKING, MSG_WORKING_UPDATE);
    //                     };
    //                     g_windMgr->showPop(POP_NORMAL, &baseData);
    //                     if (g_data->mRunStepIndex == 1) g_aligenie->textToSpeech("首段烹饪已完成，请取出食材加工");
    //                 }
    //             } else {
    //                 g_data->mStatus = ES_DOEN;
    //                 g_TimerMgr->stop(TIMER_WORKING); // 停止工作的tick
    //             }
    //             // 若不是工作页面，则需先跳转到工作页面
    //             if (g_window->getPageType() != PAGE_COOKING) {
    //                 g_windMgr->goTo(PAGE_COOKING);
    //             } else {
    //                 // 通知工作页面更新
    //                 g_windMgr->sendMsg(PAGE_COOKING, MSG_WORKING_UPDATE);
    //             }
    //             // if (g_window->getPopType() == POP_NORMAL){
    //             //     g_windMgr->closePop(POP_NORMAL);
    //             // }
    //             // 若工作状态为暂停，则是有下一步，但需弹窗提示
    //             if (g_data->mStatus == ES_WAIT) {
    //                 PopNormal::BaseDataStr baseData;
    //                 baseData.title = "温馨提示";
    //                 baseData.type = PopNormal::PT_NEXT_STEP;
    //                 baseData.info = g_data->mRunningModeData.modes[g_data->mRunStepIndex].toNextText;
    //                 baseData.enterText = "下一步";
    //                 baseData.cancelText = "";
    //                 baseData.fromView = nullptr;
    //                 baseData.enterListener = [this]() {
    //                     g_data->mRunStepIndex++;
    //                     g_data->mStatus = ES_WORKING;
    //                     g_data->mAllTime = g_data->mRunningModeData.getStepWorkTime(g_data->mRunStepIndex);
    //                     g_data->mOverTime = g_data->mAllTime;
    //                     g_TimerMgr->start(TIMER_WORKING, 1000);// 启动工作tick
    //                     g_windMgr->sendMsg(PAGE_COOKING, MSG_WORKING_UPDATE);
    //                 };

    //                 g_windMgr->showPop(POP_NORMAL, &baseData);
    //             }
    //         }
    //         g_windMgr->sendMsg(PAGE_COOKING, MSG_WORKING_TICK);
    // #ifndef TUYA_OS_DISABLE
    //         // if((g_data->getWorkingData().mode == MODE_MICRO_DEFAULT) || (g_data->getWorkingData().mode == MODE_ADDFUNS_THAW)){
    //         //     g_tuyaOsMgr->reportDpData(TYDPID_REMAIN_TIME, PROP_VALUE, &g_data->mOverTime);
    //         // }else if (mOverTimeMinute != ((g_data->mOverTime + 59) / 60)) {
    //         //     mOverTimeMinute = (g_data->mOverTime + 59) / 60;
    //         //     int reportTime = mOverTimeMinute * 60;
    //         //     g_tuyaOsMgr->reportDpData(TYDPID_REMAIN_TIME, PROP_VALUE, &reportTime);
    //         //     LOGE("afkj;ldshf");
    //         //     LOGE("mOverTimeMinute = %d g_data->mOverTime = %d",mOverTimeMinute,g_data->mOverTime);
    //         // }
    //         int reportTime = g_data->mOverTime - 1;
    //         g_tuyaOsMgr->reportDpData(TYDPID_REMAIN_TIME, PROP_VALUE, &reportTime);
    // #endif
    //     } else {
    //         g_TimerMgr->stop(TIMER_WORKING); // 停止工作的tick
    //     }
}
