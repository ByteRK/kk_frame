/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-16 09:20:42
 * @LastEditTime: 2026-01-20 09:05:08
 * @FilePath: /kk_frame/src/app/managers/statistics_mgr.cc
 * @Description: 数据统计管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "statistics_mgr.h"
#include "config_info.h"
#include "utils/json_utils.h"
#include "utils/file_utils.h"
#include <cdlog.h>

/************************* 输入输出交互，根据项目变动 *************************/

/// @brief 打包数据为Json
/// @param data 数据
/// @param json Json
inline void setOneDayDataToJson(StatisticsData& data, Json::Value& json) {
    json["count"] = data.count;
}

/// @brief 解包Json数据
/// @param data 目标
/// @param json Json
inline void setOneDayDataFromJson(StatisticsData& data, Json::Value& json) {
    data.count = JsonUtils::get<int>(json, "count");
}

/// @brief 数据叠加
/// @param data 目标数据
/// @param add 源数据
inline void addOneDayData(StatisticsData& data, StatisticsData& add) {
    data.count += add.count;
}

/*************************       结束自定义部分      *************************/

/// @brief 数据统计管理
StatisticsMgr::StatisticsMgr() : AutoSaveItem(6000, 10000) {
    mTodayIndex = 0;
    mHaveChange = false;

    mLastEventTime = time(nullptr);
    mCurrentDayEnd = getEndOfDay(mLastEventTime);
}

/// @brief 析构
StatisticsMgr::~StatisticsMgr() {
}

/// @brief 初始化
void StatisticsMgr::init() {
    load();
    AutoSaveItem::init();
}

/// @brief 复位数据
void StatisticsMgr::reset() {
    std::string command = std::string("rm")  \
        + " " + STATISTICS_FILE_PATH + " " + STATISTICS_FILE_BAK_PATH;
    std::system(command.c_str());
    FileUtils::sync();
    load();
    mHaveChange = true;
    LOGE("statistics_data factory reset.");
}

/// @brief 加载数据
/// @return 
bool StatisticsMgr::load() {
    mTodayIndex = 0;
    for (auto& day : mBuffer)
        day.reset();

    Json::Value statisticsJson;
    std::string loadingPath = "";
    size_t fileLen = 0;

    if (FileUtils::check(STATISTICS_FILE_PATH, &fileLen) && fileLen > 0) {
        loadingPath = STATISTICS_FILE_PATH;
    } else if (FileUtils::check(STATISTICS_FILE_BAK_PATH, &fileLen) && fileLen > 0) {
        loadingPath = STATISTICS_FILE_BAK_PATH;
    }

    if (loadingPath.empty() || !JsonUtils::load(loadingPath, statisticsJson)) {
        LOG(ERROR) << "[statistics] no local data file found. use null data";
        mHaveChange = true;
        return false;
    }
    LOG(INFO) << "[statistics] load local data. file=" << loadingPath;

    /**** 开始读取数据 ****/
    mTodayIndex = JsonUtils::get<int>(statisticsJson, "today_index");
    Json::Value days = statisticsJson["days"];
    for (
        uint32_t days_count = days.size(), days_index = 0;
        days_index < days_count && days_index < MAX_DAYS;
        days_index++
        ) {
        mBuffer[days_index].day_index = JsonUtils::get<int>(days[days_index], "day_index");
        setOneDayDataFromJson(mBuffer[days_index], days[days_index]);
    }

    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存数据
/// @param isBackup 是否为备份 
/// @return 保存是否成功
bool StatisticsMgr::save(bool isBackup) {
    Json::Value root;

    // 信息
    root["today_index"] = mTodayIndex; // 保存"今天"索引
    root["max_days"] = MAX_DAYS;       // 保存最大天数

    // 数据
    for (auto& day_data : mBuffer) {
        Json::Value day_json;
        day_json["day_index"] = day_data.day_index;
        setOneDayDataToJson(day_data, day_json);
        root["days"].append(day_json);
    }

    // 复位标记并保存
    mHaveChange = false;
    return JsonUtils::save(
        isBackup ? STATISTICS_FILE_BAK_PATH : STATISTICS_FILE_PATH,
        root);
}

/// @brief 获取是否有变更
/// @return 返回是否有变更
bool StatisticsMgr::haveChange() {
    checkAndRotateDay(time(nullptr));
    return mHaveChange;
}

/// @brief 获取指定日期的数据
/// @param days_ago 从今天往前第几天
/// @return 返回数据指针
const StatisticsData* StatisticsMgr::getDayData(uint16_t days_ago) {
    if (days_ago < 0 || days_ago >= MAX_DAYS)
        return nullptr;
    // 计算环形索引
    int index = (mTodayIndex + MAX_DAYS - days_ago) % MAX_DAYS;
    return &mBuffer[index];
}

/// @brief 获取指定日期往前N天的数据
/// @param days_ago 从今天往前第几天
/// @param days_count 获取几天数据
/// @return 返回一个数组，包含指定日期往前N天的数据
std::vector<const StatisticsData*> StatisticsMgr::getDayData(uint16_t days_ago, uint16_t days_count) {
    std::vector<const StatisticsData*> result;
    for (uint16_t i = 0; i < days_count; i++) {
        const StatisticsData* data = getDayData(days_ago + i);
        if (data) result.push_back(data);
    }
    return result;
}

/// @brief 获取所有的数据
/// @return 返回一个数组，包含所有的数据
std::vector<const StatisticsData*> StatisticsMgr::getAllDayData() {
    std::vector<const StatisticsData*> result;
    result.resize(mBuffer.size());
    for (auto& day_data : mBuffer) result.push_back(&day_data);
    return result;
}

/// @brief 添加一天的数据(在原基础上叠加)
/// @param data 新增数据
void StatisticsMgr::addDayData(StatisticsData& data) {
    addOneDayData(mBuffer[mTodayIndex], data);
    mHaveChange = true;
}

/// @brief 获取一天结束时间时间戳
/// @param timestamp 时间戳
/// @return 一天结束时间
time_t StatisticsMgr::getEndOfDay(time_t timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    return mktime(tm_info) + 24 * 3600;
}

/// @brief 移动到新的一天
void StatisticsMgr::rotateToNewDay() {
    // 移动"今天"指针
    mTodayIndex = (mTodayIndex + 1) % MAX_DAYS;

    // 初始化新的一天
    auto& new_day = mBuffer[mTodayIndex];
    new_day.reset();

    // 更新所有日期的相对索引
    for (size_t i = 0; i < MAX_DAYS; i++) {
        int diff = (mTodayIndex + MAX_DAYS - i) % MAX_DAYS;
        mBuffer[i].day_index = diff;
    }

    // 标记数据变更
    mHaveChange = true;
}

/// @brief 检查并移动到新的一天
/// @param current_time 当前时间戳
void StatisticsMgr::checkAndRotateDay(time_t current_time) {
    // 如果跨天了，滚动缓冲区
    if (current_time >= mCurrentDayEnd) {
        rotateToNewDay();                               // 切换到新的一天
        mCurrentDayEnd = getEndOfDay(current_time);     // 更新当前天结束时间
    }
}