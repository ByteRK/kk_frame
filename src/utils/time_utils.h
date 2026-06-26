/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-22 16:52:55
 * @LastEditTime: 2026-06-17 00:51:56
 * @FilePath: /kk_frame/src/utils/time_utils.h
 * @Description: 时间相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <cstdint>
#include <ctime>
#include <string>

namespace TimeUtils {
    static constexpr int64_t MINUTE_SECONDS = 60;                   // 分钟秒数
    static constexpr int64_t HOUR_SECONDS   = 60 * MINUTE_SECONDS;  // 小时秒数
    static constexpr int64_t DAY_SECONDS    = 24 * HOUR_SECONDS;    // 天秒数

    static constexpr int64_t MINUTE_MSEC = 1000 * MINUTE_SECONDS;   // 分钟毫秒数
    static constexpr int64_t HOUR_MSEC   = 1000 * HOUR_SECONDS;     // 小时毫秒数
    static constexpr int64_t DAY_MSEC    = 1000 * DAY_SECONDS;      // 天毫秒数

    /// @brief 判断指定年份是否为闰年
    /// @param year 年份
    /// @return true or false
    bool isLeapYear(int year);

    /// @brief 是否是有效日期
    /// @param year 年份
    /// @param month 月份
    /// @param day 日期
    /// @return true 有效日期，false 无效日期
    bool isValidDate(int year, int month, int day);

    /// @brief 是否是有效时间
    /// @param hour 小时
    /// @param minute 分钟
    /// @param second 秒
    /// @return true 有效时间，false 无效时间
    bool isValidClock(int hour, int minute, int second);

    /// @brief 将指定日期时间转换为秒级时间戳
    /// @param year 年份
    /// @param month 月份，范围 1-12
    /// @param day 日期
    /// @param hour 小时
    /// @param minute 分钟
    /// @param second 秒
    /// @return 秒级时间戳，失败返回 -1
    /// @note 输入按本地时区解释
    time_t makeTime(int year, int month, int day, int hour, int minute, int second);

    /// @brief 获取当前本地时间
    /// @return 本地时间
    /// @note 转换失败时返回 1970-01-01 00:00:00 GMT
    std::tm localTime();

    /// @brief 获取指定时间戳对应的本地时间
    /// @param timestamp 秒级时间戳
    /// @return 本地时间
    /// @note 转换失败时返回 1970-01-01 00:00:00 GMT
    std::tm localTime(const std::time_t& timestamp);

    /// @brief 获取指定时间戳对应的本地时间
    /// @param timestamp 秒级时间戳
    /// @param out 本地时间
    /// @return true or false
    /// @note 线程安全
    bool localTime(const std::time_t& timestamp, std::tm& out);

    /// @brief 获取当前本地时间
    /// @param year 年，可传 nullptr
    /// @param month 月，可传 nullptr
    /// @param day 日，可传 nullptr
    /// @param hour 时，可传 nullptr
    /// @param minute 分，可传 nullptr
    /// @param second 秒，可传 nullptr
    /// @note 时间转换失败时输出 1970-01-01 00:00:00 GMT
    void getTime(int* year, int* month, int* day, int* hour, int* minute, int* second);

    /// @brief 判断指定秒级时间戳是否为本地今天
    /// @param timestamp 秒级时间戳
    /// @return true or false
    bool isToday(const time_t& timestamp);

    /// @brief 判断两个秒级时间戳是否为同一本地日期
    /// @param lhs 第一个秒级时间戳
    /// @param rhs 第二个秒级时间戳
    /// @return true or false，时间转换失败返回 false
    bool isSameDay(const time_t& lhs, const time_t& rhs);

    /// @brief 获取秒级时间戳
    /// @return 秒级时间戳，失败返回 -1
    time_t getTimeSec();

    /// @brief 获取毫秒级时间戳
    /// @return 毫秒级时间戳，失败返回 0
    int64_t getTimeMSec();

    /// @brief 获取今天本地 0 点时间戳
    /// @return 秒级时间戳，失败返回 0
    time_t getZeroTimeSec();

    /// @brief 获取指定秒级时间戳对应的本地 0 点时间戳
    /// @param timestamp 秒级时间戳
    /// @return 秒级时间戳，失败返回 0
    time_t getZeroTimeSec(const time_t& timestamp);

    /// @brief 获取指定时间戳所在日期的下一天本地 0 点时间戳
    /// @param timestamp 秒级时间戳
    /// @return 秒级时间戳，失败返回 0
    time_t getNextZeroTimeSec(const time_t& timestamp);

    /// @brief 获取本地日期整数
    /// @return yyyymmdd，例如 20260619，失败返回 0
    int getTodayDate();

    /// @brief 获取指定时间戳对应的本地日期整数
    /// @param timestamp 秒级时间戳
    /// @return yyyymmdd，例如 20260619，失败返回 0
    int getDate(const time_t& timestamp);

    /// @brief 获取今天是星期几
    /// @return 0 表示星期日，6 表示星期六
    int getTodayDayOnWeek();

    /// @brief 获取指定时间戳是星期几
    /// @param timestamp 秒级时间戳
    /// @return 0 表示星期日，6 表示星期六，失败返回 -1
    int getDayOnWeek(const time_t& timestamp);

    /// @brief 获取时间字符串 [%H:%M]
    /// @return 时间字符串，失败返回空字符串
    std::string getTimeStr();

    /// @brief 获取当前格式化本地时间
    /// @param fmt 格式化参数，参考 strftime 格式说明符
    /// @return 格式化后的时间字符串，失败返回空字符串
    /// @note %Y-%m-%d %H:%M:%S -> 2025-03-21 12:00:00
    std::string getTimeFmtStr(const char* fmt);

    /// @brief 获取指定格式化时间
    /// @param timestamp 秒级时间戳
    /// @param fmt 格式化参数，参考 strftime 格式说明符
    /// @return 格式化后的时间字符串，失败返回空字符串
    /// @note %Y-%m-%d %H:%M:%S -> 2025-03-21 12:00:00
    std::string getTimeFmtStr(const time_t& timestamp, const char* fmt);

    /// @brief 获取当前格式化本地时间，区分上下午采用不同格式化参数
    /// @param fmtAM 上午格式化参数，小时位请使用 %I 格式化为 12 小时制
    /// @param fmtPM 下午格式化参数，小时位请使用 %I 格式化为 12 小时制
    /// @return 格式化后的时间字符串，失败返回空字符串
    std::string getTimeFmtStrAP(const char* fmtAM, const char* fmtPM);

    /// @brief 获取指定格式化时间，区分上下午采用不同格式化参数
    /// @param timestamp 秒级时间戳
    /// @param fmtAM 上午格式化参数，小时位请使用 %I 格式化为 12 小时制
    /// @param fmtPM 下午格式化参数，小时位请使用 %I 格式化为 12 小时制
    /// @return 格式化后的时间字符串，失败返回空字符串
    std::string getTimeFmtStrAP(const time_t& timestamp, const char* fmtAM, const char* fmtPM);

    /// @brief 获取 time_t 能安全表示的最大年份，不超过 upperLimit
    /// @param upperLimit 上限
    /// @return 最大年份
    int getMaxYear(int upperLimit = 2099);

    /// @brief 获取指定年份指定月份的最大天数
    /// @param year 年
    /// @param month 月，范围 1-12
    /// @return 最大天数，month 非法返回 0
    int getMaxDay(int year, int month);

    /// @brief 获取今天的星期文本
    /// @return 星期几文本
    std::string getDayOnWeek();

    /// @brief 获取星期文本
    /// @param day 星期索引，0 表示星期日，按模 7 归一化
    /// @return 星期几文本
    std::string getDayOnWeek(const int& day);
}

#endif // TIME_UTILS_H
