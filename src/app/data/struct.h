/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2026-01-19 17:00:39
 * @FilePath: /kk_frame/src/app/data/struct.h
 * @Description: 数据结构定义
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __STRUCT_H__
#define __STRUCT_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <list>

/// @brief 历史数据结构体
typedef struct HistoryStruct_t {
    std::string title;     // 标题
    std::string status;    // 状态
    std::string time;      // 时间
} HistoryStruct;

/// @brief 统计数据结构体
typedef struct StatisticsData_t {
    uint32_t day_index;               // 相对天数[固定项]（0=今天，1=昨天，...）
    uint16_t count;                   // 运行次数

    void reset() {
        day_index = 0;
        count = 0;
    }
} StatisticsData;

#endif // __STRUCT_H__