/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-09-01 18:00:29
 * @LastEditTime: 2025-09-01 18:13:50
 * @FilePath: /kk_frame/src/project/work_timer.cc
 * @Description: 内置定时器列表
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "work_timer.h"

// #include "global_data.h"
// #include "base.h"
// #include "proto.h"
// #include "wind_mgr.h"


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
