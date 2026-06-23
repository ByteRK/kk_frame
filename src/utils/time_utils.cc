/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-22 16:52:55
 * @LastEditTime: 2026-06-23 11:49:30
 * @FilePath: /kk_frame/src/utils/time_utils.cc
 * @Description: 时间相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "time_utils.h"
#include <cdlog.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <ctime>
#include <sys/time.h>

namespace {
    /// @brief 按 fmt 格式化已经转换好的 tm
    std::string formatTm(const std::tm& tmTime, const char* fmt) {
        if (fmt == nullptr || *fmt == '\0') {
            LOGE("formatTm invalid fmt");
            return std::string();
        }

        char buffer[128] = { 0 };
        const std::size_t len = std::strftime(buffer, sizeof(buffer), fmt, &tmTime);
        if (len == 0) {
            LOGE("strftime failed, fmt=%s", fmt);
            return std::string();
        }

        return std::string(buffer, len);
    }

    /// @brief 格式化秒级时间戳
    std::string formatTime(const std::time_t& timestamp, const char* fmt) {
        std::tm tmTime = {};
        if (!TimeUtils::localTime(timestamp, tmTime)) {
            LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
            return std::string();
        }

        return formatTm(tmTime, fmt);
    }
}

bool TimeUtils::isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

bool TimeUtils::isValidDate(int year, int month, int day) {
    if (year <= 0 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    static const int kDays[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    int maxDay = kDays[month - 1];
    if (month == 2 && isLeapYear(year)) {
        maxDay = 29;
    }

    return day <= maxDay;
}

bool TimeUtils::isValidClock(int hour, int minute, int second) {
    return hour >= 0 && hour <= 23
        && minute >= 0 && minute <= 59
        && second >= 0 && second <= 59;
}

std::tm TimeUtils::localTime(const std::time_t & timestamp) {
    std::tm out = {};
    localTime(timestamp, out);
    return out;
}

bool TimeUtils::localTime(const std::time_t & timestamp, std::tm & out) {
#if defined(_WIN32)
    return localtime_s(&out, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &out) != nullptr;
#endif
}

void TimeUtils::getTime(int* year, int* month, int* day, int* hour, int* minute, int* second) {
    const std::time_t t = getTimeSec();
    if (t == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return;
    }

    std::tm cur = {};
    if (!localTime(t, cur)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(t));
        return;
    }

    if (year)   *year = cur.tm_year + 1900;
    if (month)  *month = cur.tm_mon + 1;
    if (day)    *day = cur.tm_mday;
    if (hour)   *hour = cur.tm_hour;
    if (minute) *minute = cur.tm_min;
    if (second) *second = cur.tm_sec;
}

bool TimeUtils::isToday(const time_t& timestamp) {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return false;
    }

    std::tm nowTm = {};
    std::tm targetTm = {};

    if (!localTime(now, nowTm)) {
        LOGE("localTime now failed, timestamp=%lld", static_cast<long long>(now));
        return false;
    }

    if (!localTime(timestamp, targetTm)) {
        LOGE("localTime target failed, timestamp=%lld", static_cast<long long>(timestamp));
        return false;
    }

    return nowTm.tm_year == targetTm.tm_year && nowTm.tm_yday == targetTm.tm_yday;
}

time_t TimeUtils::getTimeSec() {
    return std::time(nullptr);
}

int64_t TimeUtils::getTimeMSec() {
    timeval tv = {};
    if (gettimeofday(&tv, nullptr) != 0) {
        LOGE("gettimeofday failed, errno=%d", errno);
        return 0;
    }

    return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

time_t TimeUtils::getZeroTimeSec() {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return 0;
    }
    return getZeroTimeSec(now);
}

time_t TimeUtils::getZeroTimeSec(const time_t & timestamp) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return 0;
    }

    tmTime.tm_hour = 0;
    tmTime.tm_min = 0;
    tmTime.tm_sec = 0;
    tmTime.tm_isdst = -1;

    const std::time_t zero = std::mktime(&tmTime);
    if (zero == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while getZeroTimeSec");
        return 0;
    }

    return zero;
}

int TimeUtils::getTodayDate() {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return 0;
    }

    return getDate(now);
}

int TimeUtils::getDate(const time_t& timestamp) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return 0;
    }

    return (tmTime.tm_year + 1900) * 10000
        + (tmTime.tm_mon + 1) * 100
        + tmTime.tm_mday;
}

std::string TimeUtils::getTimeStr() {
    return getTimeFmtStr("%H:%M");
}

std::string TimeUtils::getTimeFmtStr(const char* fmt) {
    const std::time_t timestamp = getTimeSec();
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return std::string();
    }

    return getTimeFmtStr(timestamp, fmt);
}

std::string TimeUtils::getTimeFmtStr(const std::time_t& timestamp, const char* fmt) {
    return formatTime(timestamp, fmt);
}

std::string TimeUtils::getTimeFmtStrAP(const char* fmtAM, const char* fmtPM) {
    const std::time_t timestamp = getTimeSec();
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return std::string();
    }

    return getTimeFmtStrAP(timestamp, fmtAM, fmtPM);
}

std::string TimeUtils::getTimeFmtStrAP(const std::time_t& timestamp, const char* fmtAM, const char* fmtPM) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return std::string();
    }

    const char* fmt = tmTime.tm_hour < 12 ? fmtAM : fmtPM;
    return formatTm(tmTime, fmt);
}

int TimeUtils::getMaxYear(int upperLimit) {
    int year = 2036;

    if (sizeof(time_t) >= 8) {
        // 64 位 time_t 可表示范围极大，这里只受 upperLimit 限制
        year = upperLimit;
    } else if (sizeof(time_t) == 4) {
        // 32 位 time_t 最大安全年份接近 2038，这里保守返回 2037
        year = 2037;
    }

    return std::min(year, upperLimit);
}

int TimeUtils::getMaxDay(int year, int month) {
    if (month < 1 || month > 12) {
        return 0;
    }

    static const int days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month == 2 && isLeapYear(year)) {
        return 29;
    }

    return days[month - 1];
}

std::string TimeUtils::getDayOnWeek(const int& day) {
    static const std::string weekList[7] = {
        "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
    };

    int index = day % 7;
    if (index < 0) {
        index += 7;
    }

    return weekList[index];
}
