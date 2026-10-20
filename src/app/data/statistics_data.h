/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-19 10:00:00
 * @LastEditTime: 2026-10-20 09:19:45
 * @FilePath: /kk_frame/src/app/data/statistics_data.h
 * @Description: 统计数据与实例
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
 *    项目自定义：
 *      - 直接修改下方 StatisticsData 字段
 *      - 同步修改 StatisticsTraits 中的序列化、叠加与保留天数
 *      - 通用逻辑在 src/class/statistics_store.h，无需改动
 *
**/

#ifndef __STATISTICS_DATA_H__
#define __STATISTICS_DATA_H__

#include "class/statistics_store.h"
#include "config_info.h"

/// @brief 统计数据
/// @note 日期由存储模板统一管理，此处不需要日期字段
/// @note 项目把示例字段替换为实际业务字段
typedef struct StatisticsData_t {
    bool empty = false; // 示例字段，项目自行替换
} StatisticsData;

/// @brief 统计数据项目策略
struct StatisticsTraits {
    typedef StatisticsData Data;

    static const char*  filePath() { return STATISTICS_FILE_PATH; }
    static const char*  backupPath() { return STATISTICS_FILE_BAK_PATH; }
    static size_t       maxKeepDays() { return 365; }  // 保留1年
    static uint32_t     checkInterval() { return 6000; }
    static uint32_t     backupInterval() { return 10000; }

    static void toJson(const Data& data, Json::Value& json) {
        json["empty"] = data.empty;
    }

    static void fromJson(Data& data, const Json::Value& json) {
        data.empty = JsonUtils::get<bool>(json, "empty", false);
    }

    static void merge(Data& data, const Data& add) {
        (void)data;
        (void)add;
    }
};

/// @brief 数据统计存储
#define g_statistics StatisticsStore<StatisticsTraits>::instance()

#endif // !__STATISTICS_DATA_H__
