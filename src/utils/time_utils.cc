/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-22 16:52:55
 * @LastEditTime: 2025-12-30 17:41:20
 * @FilePath: /kk_frame/src/utils/time_utils.cc
 * @Description: 时间相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "time_utils.h"
#include <cdlog.h>
#include <chrono>
#include <sys/time.h>

bool TimeUtils::isToday(const time_t& timestamp) {
    time_t zeroSec = getZeroTimeSec();
    return zeroSec <= timestamp && zeroSec + DAY_SECONDS > timestamp;
}

int64_t TimeUtils::getTimeSec() {
    struct timeval _val;
    gettimeofday(&_val, NULL);
    return _val.tv_sec;
}

int64_t TimeUtils::getTimeMSec() {
    struct timeval _val;
    gettimeofday(&_val, NULL);
    return (int64_t)_val.tv_sec * 1000 + _val.tv_usec / 1000;
}

time_t TimeUtils::getZeroTimeSec() {
    time_t     t = time(NULL);
    struct tm* tm = localtime(&t);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    return mktime(tm);
}

std::string TimeUtils::getTimeStr() {
#if 0
    char           buff[128];
    std::time_t    t = std::time(NULL);
    struct std::tm cur = *std::localtime(&t);
    snprintf(buff, sizeof(buff), "%02d:%02d", cur.tm_hour, cur.tm_min);
    return std::string(buff);
#else
    return getTimeFmtStr("MM:SS");
#endif
}

std::string TimeUtils::getTimeFmtStr(const char* fmt) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    return getTimeFmtStr(now_c, fmt);
}

std::string TimeUtils::getTimeFmtStr(const time_t& timestamp, const char* fmt) {
    std::tm* time_tm = std::localtime(&timestamp);
    char     datetime[128];
    strftime(datetime, sizeof(datetime), fmt, time_tm);
    return   datetime;
}

std::string TimeUtils::getTimeFmtStrAP(const char* fmtAM, const char* fmtPM) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    return getTimeFmtStrAP(now_c, fmtAM, fmtPM);
}

std::string TimeUtils::getTimeFmtStrAP(const time_t& timestamp, const char* fmtAM, const char* fmtPM) {
    std::tm* time_tm = std::localtime(&timestamp);
    char     datetime[128];
    // 屏蔽，通过格式化小时参数为%I进行自动计算
    // time_tm->tm_hour = time_tm->tm_hour > 0 ? time_tm->tm_hour -= 12 : 12;
    strftime(datetime, sizeof(datetime), (time_tm->tm_hour >= 12 ? fmtAM : fmtPM), time_tm);
    return datetime;
}

int TimeUtils::getMaxDay(int year, int month) {
    if (month < 1 || month > 12) return 0;
#if 0 // 标准库方式
    tm tm_struct = {};                // 初始化 tm 结构体
    tm_struct.tm_year = year - 1900;  // tm_year 是从 1900 开始的年份偏移
    tm_struct.tm_mon = month;         // 设置为目标月份的下一个月（例如：3月 -> 4月）
    tm_struct.tm_mday = 0;            // 设置为下个月的“第0天”，即上个月的最后一天
    tm_struct.tm_hour = 0;            // 必须设置时间字段，避免未定义行为
    mktime(&tm_struct);               // 标准化时间，自动调整日期
    return tm_struct.tm_mday;         // 返回计算后的天数
#else // 查表方式
    static const int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) {
        // 判断闰年条件：能被4整除但不能被100整除，或能被400整除
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29; // 闰年2月有29天
        } else {
            return 28; // 平年2月有28天
        }
    } else {
        return days[month - 1];       // 月份从1开始，数组索引从0开始
    }
#endif
}

std::string TimeUtils::getDayOnWeek(const int& day) {
    static const std::string weekList[7] = { "星期日","星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    return weekList[day % 7];
}

void TimeUtils::setTime(const int64_t& timestamp) {
#ifndef CDROID_X64
    struct timeval set_tv;
    set_tv.tv_sec = timestamp;
    set_tv.tv_usec = 0;
    settimeofday(&set_tv, NULL);
    LOGE("set time %s", getTimeFmtStr("%Y-%m-%d %H:%M:%S").c_str());
#ifdef CDROID_SIGMA
    sysCommand("hwclock --systohc");
#endif
#else
    LOGE("set time %s", getTimeFmtStr(timestamp, "%Y-%m-%d %H:%M:%S").c_str());
#endif
}

void TimeUtils::setTime(int year, int month, int day, int hour, int minute, int second) {
    // 数据有效性检查
    if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
        return;
    }
    // 使用 std::chrono::system_clock 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm cur = *std::localtime(&currentTime);
    // 更新时间信息 (仅当参数有效时更新)
    if (year > 0) cur.tm_year = year - 1900;              // Year - 1900.
    if (month > 0 && month <= 12) cur.tm_mon = month - 1; // [0-11]
    if (day > 0) cur.tm_mday = day;                       // [1-31]
    if (hour >= 0) cur.tm_hour = hour;                    // [0-23]
    if (minute >= 0) cur.tm_min = minute;                 // [0-59]
    if (second >= 0) cur.tm_sec = second;                 // [0-60]
    // 处理日期溢出
    std::time_t newTime = std::mktime(&cur);
    if (newTime == -1) {
        LOGE("Invalid date/time provided.");
        return;
    }
    // 设置系统时间
    setTime(newTime);
}
