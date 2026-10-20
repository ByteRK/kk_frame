/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-19 10:00:00
 * @LastEditTime: 2026-10-20 09:20:16
 * @FilePath: /kk_frame/src/app/data/history_data.h
 * @Description: 历史记录数据与实例
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
 *    项目自定义：
 *      - 直接修改下方 HistoryStruct 字段
 *      - 同步修改 HistoryTraits 中的序列化与容量
 *      - 通用逻辑在 src/class/history_store.h，无需改动
 *
**/

#ifndef __HISTORY_DATA_H__
#define __HISTORY_DATA_H__

#include "class/history_store.h"
#include "config_info.h"

/// @brief 历史记录数据
typedef struct HistoryStruct_t {
    bool empty = false;
} HistoryStruct;

/// @brief 历史记录策略
struct HistoryTraits {
    typedef HistoryStruct Data;

    static const char*  filePath() { return HISTORY_FILE_PATH; }
    static const char*  backupPath() { return HISTORY_FILE_BAK_PATH; }
    static size_t       maxCount() { return 80; }      // 最多80条
    static uint32_t     checkInterval() { return 8000; }
    static uint32_t     backupInterval() { return 10000; }

    static void toJson(const Data& data, Json::Value& json) {
        json["empty"] = data.empty;
    }

    static void fromJson(Data& data, const Json::Value& json) {
        data.empty = JsonUtils::get<bool>(json, "empty", false);
    }
};

/// @brief 历史记录存储
#define g_history HistoryStore<HistoryTraits>::instance()

#endif // !__HISTORY_DATA_H__
