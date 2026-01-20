/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-16 09:20:32
 * @LastEditTime: 2026-01-19 18:39:13
 * @FilePath: /kk_frame/src/app/managers/statistics_mgr.h
 * @Description: 数据统计管理
 * 
 *    循环缓冲区结构->
 *      |-------------------------------------------------------|
 *      |   3   |   2   |   1   |   0   |   6   |   5   |   4   |
 *      |-------------------------------------------------------|
 *      |   3   |大前天 | 前一天|  当前 |   6   |   5   |   4   |
 *      |-------------------------------------------------------|
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __STATISTICS_MGR_H__
#define __STATISTICS_MGR_H__

#include "struct.h"
#include "template/singleton.h"
#include "class/auto_save.h"
#include <array>

#define g_statistics StatisticsMgr::instance()

class StatisticsMgr :
    public AutoSaveItem,
    public Singleton<StatisticsMgr> {
    friend Singleton<StatisticsMgr>;
private:
    static constexpr size_t              MAX_DAYS = 365;        // 存储1年数据
    std::array<StatisticsData, MAX_DAYS> mBuffer;               // 环形数据缓冲区
    uint32_t                             mTodayIndex;           // "今天"索引
    bool                                 mHaveChange;           // 数据是否发生变化

private:
    time_t                               mLastEventTime;        // 上次事件时间
    time_t                               mCurrentDayEnd;        // 当天结束时间
    
private:
    StatisticsMgr();

public:
    ~StatisticsMgr();
    void            init();
    void            reset();
    bool            load();
    bool            save(bool isBackup = false) override;
    bool            haveChange() override;

public:
    std::vector<const StatisticsData*>
                    getAllDayData();
    const StatisticsData* 
                    getDayData(uint16_t days_ago);
    std::vector<const StatisticsData*> 
                    getDayData(uint16_t days_ago, uint16_t days_count);
    void            addDayData(StatisticsData& data);
    
private:
    time_t          getEndOfDay(time_t timestamp);
    void            rotateToNewDay();
    void            checkAndRotateDay(time_t current_time);
};

#endif // !__STATISTICS_MGR_H__
