/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-04 14:00:00
 * @FilePath: /kk_frame/src/utils/platform/clock_utils.cc
 * @Description: 系统时钟相关的一些函数（设置时间、硬件时钟同步、单调时钟）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "clock_utils.h"

#include "time_utils.h"
#include "process_utils.h"

#include <cdlog.h>

#include <cerrno>
#include <ctime>
#include <sys/time.h>
#include <time.h>

bool ClockUtils::setTime(const int64_t& timestamp) {
#ifndef PRODUCT_X64
    timeval setTv = {};
    setTv.tv_sec = static_cast<time_t>(timestamp);
    setTv.tv_usec = 0;

    if (settimeofday(&setTv, nullptr) != 0) {
        LOGE("settimeofday failed, timestamp=%lld errno=%d",
            static_cast<long long>(timestamp), errno);
        return false;
    }

    syncHWClock();
    LOGI("set time %s", TimeUtils::getTimeFmtStr(static_cast<time_t>(timestamp), "%Y-%m-%d %H:%M:%S").c_str());
    return true;
#else
    LOGI("set time %s", TimeUtils::getTimeFmtStr(static_cast<time_t>(timestamp), "%Y-%m-%d %H:%M:%S").c_str());
    return false;
#endif
}

bool ClockUtils::setTime(int year, int month, int day, int hour, int minute, int second) {
    if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
        return false;
    }

    const std::time_t currentTime = TimeUtils::getTimeSec();
    if (currentTime == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return false;
    }

    std::tm cur = {};
    if (!TimeUtils::localTime(currentTime, cur)) {
        LOGE("localTime failed, timestamp=%lld", static_cast<long long>(currentTime));
        return false;
    }

    if (year > 0) {
        cur.tm_year = year - 1900;
    }

    if (month > 0) {
        if (month > 12) {
            LOGE("Invalid month=%d", month);
            return false;
        }
        cur.tm_mon = month - 1;
    }

    if (day > 0) {
        cur.tm_mday = day;
    }

    if (hour >= 0) {
        if (hour > 23) {
            LOGE("Invalid hour=%d", hour);
            return false;
        }
        cur.tm_hour = hour;
    }

    if (minute >= 0) {
        if (minute > 59) {
            LOGE("Invalid minute=%d", minute);
            return false;
        }
        cur.tm_min = minute;
    }

    if (second >= 0) {
        if (second > 59) {
            LOGE("Invalid second=%d", second);
            return false;
        }
        cur.tm_sec = second;
    }

    const int finalYear = cur.tm_year + 1900;
    const int finalMonth = cur.tm_mon + 1;
    const int finalDay = cur.tm_mday;

    if (!TimeUtils::isValidDate(finalYear, finalMonth, finalDay)) {
        LOGE("Invalid date year=%d month=%d day=%d", finalYear, finalMonth, finalDay);
        return false;
    }

    if (!TimeUtils::isValidClock(cur.tm_hour, cur.tm_min, cur.tm_sec)) {
        LOGE("Invalid time hour=%d minute=%d second=%d", cur.tm_hour, cur.tm_min, cur.tm_sec);
        return false;
    }

    cur.tm_isdst = -1;

    const std::time_t newTime = std::mktime(&cur);
    if (newTime == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while setTime");
        return false;
    }

    return setTime(static_cast<int64_t>(newTime));
}

void ClockUtils::syncHWClock() {
#ifdef PRODUCT_SIGMA
    ProcessUtils::runAsync("hwclock --systohc");
#endif
}

int64_t ClockUtils::monotonicMs() {
    struct timespec ts = {};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
    }

    LOGE("clock_gettime(CLOCK_MONOTONIC) failed, errno=%d", errno);

    // 退化：返回时间仍是"单调递增"的墙钟。
    // 绝不能返回 0：调用方（如 ProcessUtils 的超时计算）会用它做差值，
    // 返回 0 会让 deadline 变成天文数字，剩余时长强转 int 后溢出。
    const int64_t wallMs = TimeUtils::getTimeMSec();
    return wallMs > 0 ? wallMs : 1;
}
