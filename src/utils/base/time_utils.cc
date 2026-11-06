/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-22 16:52:55
 * @LastEditTime: 2026-11-05 19:00:00
 * @FilePath: /kk_frame/src/utils/base/time_utils.cc
 * @Description: 时间域工具（读时钟、日历换算、时间/时长格式化）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "time_utils.h"

#include "string_utils.h"

#include <cdlog.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <ctime>
#include <sys/time.h>

namespace TimeUtils {
    const int64_t MINUTE_SECONDS = 60;
    const int64_t HOUR_SECONDS = 60 * MINUTE_SECONDS;
    const int64_t DAY_SECONDS = 24 * HOUR_SECONDS;

    const int64_t MINUTE_MSEC = 1000 * MINUTE_SECONDS;
    const int64_t HOUR_MSEC = 1000 * HOUR_SECONDS;
    const int64_t DAY_MSEC = 1000 * DAY_SECONDS;
} // namespace TimeUtils

namespace {

    /// @brief 获取 Unix epoch 对应的格林时间
    std::tm getMinGreenwichTime() {
        std::tm time = {};
        time.tm_year = 70;
        time.tm_mon = 0;
        time.tm_mday = 1;
        time.tm_wday = 4;
        return time;
    }

} // namespace

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

std::tm TimeUtils::localTime() {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        return getMinGreenwichTime();
    }

    return localTime(now);
}

std::tm TimeUtils::localTime(const std::time_t& timestamp) {
    std::tm out = {};
    return localTime(timestamp, out) ? out : getMinGreenwichTime();
}

bool TimeUtils::localTime(const std::time_t& timestamp, std::tm& out) {
#if defined(_WIN32)
    return localtime_s(&out, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &out) != nullptr;
#endif
}

void TimeUtils::getTime(int* year, int* month, int* day, int* hour, int* minute, int* second) {
    const std::tm cur = localTime();

    if (year)   *year = cur.tm_year + 1900;
    if (month)  *month = cur.tm_mon + 1;
    if (day)    *day = cur.tm_mday;
    if (hour)   *hour = cur.tm_hour;
    if (minute) *minute = cur.tm_min;
    if (second) *second = cur.tm_sec;
}

/* ============================================================================
 * 日历换算
** ========================================================================== */

bool TimeUtils::isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

int TimeUtils::getMaxDay(int year, int month) {
    if (month < 1 || month > 12) {
        return 0;
    }

    static const int kDays[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month == 2 && isLeapYear(year)) {
        return 29;
    }

    return kDays[month - 1];
}

bool TimeUtils::isValidDate(int year, int month, int day) {
    if (year <= 0 || day < 1) {
        return false;
    }

    // 月份天数与闰年判定统一由 getMaxDay 提供，避免两处月份表重复
    const int maxDay = getMaxDay(year, month);
    return maxDay > 0 && day <= maxDay;
}

bool TimeUtils::isValidClock(int hour, int minute, int second) {
    return hour >= 0 && hour <= 23
        && minute >= 0 && minute <= 59
        && second >= 0 && second <= 59;
}

time_t TimeUtils::makeTime(int year, int month, int day, int hour, int minute, int second) {
    if (!isValidDate(year, month, day) || !isValidClock(hour, minute, second)) {
        LOGE("invalid date time %d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);
        return static_cast<time_t>(-1);
    }

    std::tm tmTime = {};
    tmTime.tm_year = year - 1900;
    tmTime.tm_mon = month - 1;
    tmTime.tm_mday = day;
    tmTime.tm_hour = hour;
    tmTime.tm_min = minute;
    tmTime.tm_sec = second;
    tmTime.tm_isdst = -1;

    const std::time_t timestamp = std::mktime(&tmTime);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while makeTime");
    }

    return timestamp;
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

int TimeUtils::getDateDaysAgo(uint16_t daysAgo) {
    std::tm tmTime = localTime();
    tmTime.tm_mday -= static_cast<int>(daysAgo);
    tmTime.tm_hour = 0;
    tmTime.tm_min = 0;
    tmTime.tm_sec = 0;
    tmTime.tm_isdst = -1;

    // mktime 负责归一化跨月、闰年以及夏令时
    const std::time_t zero = std::mktime(&tmTime);
    if (zero == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while getDateDaysAgo, daysAgo=%d", static_cast<int>(daysAgo));
        return 0;
    }

    return getDate(zero);
}

time_t TimeUtils::getZeroTimeSec() {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return 0;
    }
    return getZeroTimeSec(now);
}

time_t TimeUtils::getZeroTimeSec(const time_t& timestamp) {
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

time_t TimeUtils::getNextZeroTimeSec(const time_t& timestamp) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return 0;
    }

    tmTime.tm_mday += 1;
    tmTime.tm_hour = 0;
    tmTime.tm_min = 0;
    tmTime.tm_sec = 0;
    tmTime.tm_isdst = -1;

    const std::time_t nextZero = std::mktime(&tmTime);
    if (nextZero == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while getNextZeroTimeSec");
        return 0;
    }

    return nextZero;
}

bool TimeUtils::isToday(const time_t& timestamp) {
    const std::time_t now = getTimeSec();
    if (now == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return false;
    }

    return isSameDay(now, timestamp);
}

bool TimeUtils::isSameDay(const time_t& lhs, const time_t& rhs) {
    std::tm lhsTm = {};
    std::tm rhsTm = {};

    if (!localTime(lhs, lhsTm) || !localTime(rhs, rhsTm)) {
        LOGE("localTime failed, lhs=%lld rhs=%lld",
            static_cast<long long>(lhs), static_cast<long long>(rhs));
        return false;
    }

    return lhsTm.tm_year == rhsTm.tm_year && lhsTm.tm_yday == rhsTm.tm_yday;
}

int TimeUtils::getTodayDayOnWeek() {
    const std::tm tmTime = localTime();
    return tmTime.tm_wday;
}

int TimeUtils::getDayOnWeek(const time_t& timestamp) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return -1;
    }

    return tmTime.tm_wday;
}

std::string TimeUtils::getDayOnWeek() {
    return getDayOnWeek("星期");
}

std::string TimeUtils::getDayOnWeek(const std::string& prefix) {
    return getDayOnWeek(getTodayDayOnWeek(), prefix);
}

std::string TimeUtils::getDayOnWeek(int day) {
    return getDayOnWeek(day, "星期");
}

std::string TimeUtils::getDayOnWeek(int day, const std::string& prefix) {
    static const std::string kWeekList[7] = {
        "日", "一", "二", "三", "四", "五", "六"
    };

    // 负数（如 getDayOnWeek(timestamp) 的失败值 -1）不能归一化成"星期六"，
    // 必须显式报错，避免把失败当成有效结果展示
    if (day < 0) {
        LOGW("getDayOnWeek: invalid day=%d", day);
        return std::string();
    }

    return prefix + kWeekList[day % 7];
}

/* ============================================================================
 * 格式化
** ========================================================================== */

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

} // namespace

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

std::string TimeUtils::getTimeFmtStrAP(const std::time_t& timestamp,
    const char* fmtAM, const char* fmtPM) {
    std::tm tmTime = {};
    if (!localTime(timestamp, tmTime)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(timestamp));
        return std::string();
    }

    const char* fmt = tmTime.tm_hour < 12 ? fmtAM : fmtPM;
    return formatTm(tmTime, fmt);
}

std::string TimeUtils::formatDuration(int64_t seconds, bool showDays) {
    if (seconds < 0) seconds = 0;

    const int64_t days = seconds / DAY_SECONDS;
    const int64_t hours = (seconds % DAY_SECONDS) / HOUR_SECONDS;
    const int64_t minutes = (seconds % HOUR_SECONDS) / MINUTE_SECONDS;
    const int64_t secs = seconds % MINUTE_SECONDS;

    if (showDays) {
        if (days > 0) {
            return StringUtils::format("%lld天 %02lld:%02lld:%02lld",
                static_cast<long long>(days),
                static_cast<long long>(hours),
                static_cast<long long>(minutes),
                static_cast<long long>(secs));
        }
        return StringUtils::format("%02lld:%02lld:%02lld",
            static_cast<long long>(hours),
            static_cast<long long>(minutes),
            static_cast<long long>(secs));
    }

    // 不显示天数时，小时按总小时数展示（可超过 24）
    return StringUtils::format("%02lld:%02lld:%02lld",
        static_cast<long long>(seconds / HOUR_SECONDS),
        static_cast<long long>(minutes),
        static_cast<long long>(secs));
}
