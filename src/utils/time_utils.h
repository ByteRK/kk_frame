/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-22 16:52:55
 * @LastEditTime: 2026-01-04 09:18:30
 * @FilePath: /kk_frame/src/utils/time_utils.h
 * @Description: 时间相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TIME_UTILS_H__
#define __TIME_UTILS_H__

#include <cstdint>
#include <string>

namespace TimeUtils {
    const int DAY_SECONDS = 86400; // 一天的秒数
    const int HOUR_SECONDS = 3600; // 一小时的秒数
    const int MINUTE_SECONDS = 60; // 一分钟的秒数

    /// @brief 判断是否是今天
    /// @param timestamp 时间戳
    /// @return true or false
    bool isToday(const time_t& timestamp);

    /// @brief 获取秒级时间戳
    /// @return 秒级时间戳
    int64_t getTimeSec();

    /// @brief 获取毫秒级时间戳
    /// @return 毫秒级时间戳
    int64_t getTimeMSec();

    /// @brief 获取0点时间戳
    /// @return 0点时间戳
    time_t getZeroTimeSec();

    /// @brief 获取时间字符串 [%H:%M]
    /// @return 时间字符串
    std::string getTimeStr();

    /// @brief 获取当前格式化时间
    /// @param fmt 格式化参数(具体搜索strftime格式说明符)
    /// @return 格式化后的时间字符串
    /// @note %Y-%m-%d %H:%M:%S->2025-03-21 12:00:00
    std::string getTimeFmtStr(const char* fmt);

    /// @brief 获取指定格式化时间
    /// @param timestamp 时间戳 s
    /// @param fmt 格式化参数(具体搜索strftime格式说明符)
    /// @return 格式化后的时间字符串
    /// @note %Y-%m-%d %H:%M:%S->2025-03-21 12:00:00
    std::string getTimeFmtStr(const time_t& timestamp, const char* fmt);

    /// @brief 获取当前格式化时间 (区分上下午采用不同的格式化参数)
    /// @param fmtAM 上午格式化参数 (小时位请使用%I以格式化为12小时制)
    /// @param fmtPM 下午格式化参数 (小时位请使用%I以格式化为12小时制）
    /// @return 格式化后的时间字符串
    std::string getTimeFmtStrAP(const char* fmtAM, const char* fmtPM);

    /// @brief 获取指定格式化时间 (区分上下午采用不同的格式化参数)
    /// @param timestamp 时间戳 s
    /// @param fmtAM 上午格式化参数 (小时位请使用%I以格式化为12小时制)
    /// @param fmtPM 下午格式化参数 (小时位请使用%I以格式化为12小时制）
    /// @return 格式化后的时间字符串
    std::string getTimeFmtStrAP(const time_t& timestamp, const char* fmtAM, const char* fmtPM);

    /// @brief 获取指定年份指定月份的最大天数
    /// @param year 年
    /// @param month 月
    /// @return 最大天数
    int getMaxDay(int year, int month);

    /// @brief 获取星期文本
    /// @param day 星期几
    /// @return 星期几文本
    std::string getDayOnWeek(const int& day);

    /// @brief 设置时间
    /// @param timestamp 时间戳
    void setTime(const int64_t& timestamp);

    /// @brief 设置时间
    /// @param year 年
    /// @param month 月
    /// @param day 日
    /// @param hour 时
    /// @param minute 分
    /// @param second 秒
    void setTime(int year, int month, int day, int hour, int minute, int second = 0);

}

#endif // !__TIME_UTILS_H__