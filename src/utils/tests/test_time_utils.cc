/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/test_time_utils.cc
 * @Description: TimeUtils 用例（当前时刻 / 日历换算 / 格式化）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "time_utils.h"

TEST_CASE(calendar_leapYear) {
    EXPECT_TRUE(TimeUtils::isLeapYear(2000));
    EXPECT_TRUE(TimeUtils::isLeapYear(2024));
    EXPECT_FALSE(TimeUtils::isLeapYear(1900));
    EXPECT_FALSE(TimeUtils::isLeapYear(2023));
}

TEST_CASE(calendar_maxDay) {
    EXPECT_EQ(TimeUtils::getMaxDay(2023, 1), 31);
    EXPECT_EQ(TimeUtils::getMaxDay(2023, 2), 28);
    EXPECT_EQ(TimeUtils::getMaxDay(2024, 2), 29);
    EXPECT_EQ(TimeUtils::getMaxDay(2024, 4), 30);
    EXPECT_EQ(TimeUtils::getMaxDay(2024, 13), 0);
    EXPECT_EQ(TimeUtils::getMaxDay(2024, 0), 0);
}

TEST_CASE(calendar_validDateClock) {
    EXPECT_TRUE(TimeUtils::isValidDate(2024, 2, 29));
    EXPECT_FALSE(TimeUtils::isValidDate(2023, 2, 29));
    EXPECT_FALSE(TimeUtils::isValidDate(2024, 4, 31));
    EXPECT_FALSE(TimeUtils::isValidDate(0, 1, 1));
    EXPECT_FALSE(TimeUtils::isValidDate(2024, 1, 0));

    EXPECT_TRUE(TimeUtils::isValidClock(0, 0, 0));
    EXPECT_TRUE(TimeUtils::isValidClock(23, 59, 59));
    EXPECT_FALSE(TimeUtils::isValidClock(24, 0, 0));
    EXPECT_FALSE(TimeUtils::isValidClock(0, 60, 0));
    EXPECT_FALSE(TimeUtils::isValidClock(0, 0, 60));
}

TEST_CASE(calendar_makeTimeRoundTrip) {
    const time_t stamp = TimeUtils::makeTime(2024, 6, 19, 12, 30, 45);
    EXPECT_TRUE(stamp > 0);
    EXPECT_EQ(TimeUtils::getDate(stamp), 20240619);
    EXPECT_EQ(TimeUtils::getDayOnWeek(stamp) >= 0, true);

    // 非法输入返回 -1
    EXPECT_TRUE(TimeUtils::makeTime(2023, 2, 29, 0, 0, 0) == static_cast<time_t>(-1));
}

TEST_CASE(calendar_zeroTime) {
    const time_t stamp = TimeUtils::makeTime(2024, 6, 19, 15, 20, 30);
    const time_t zero = TimeUtils::getZeroTimeSec(stamp);
    EXPECT_TRUE(zero > 0);
    EXPECT_TRUE(zero < stamp);

    // 当天 0 点应仍在同一天
    EXPECT_TRUE(TimeUtils::isSameDay(zero, stamp));

    const time_t nextZero = TimeUtils::getNextZeroTimeSec(stamp);
    EXPECT_TRUE(nextZero > zero);
    // 下一天 0 点与当天 0 点相差一个自然日（夏令时地区可能为 23/25 小时）
    const int64_t span = static_cast<int64_t>(nextZero - zero);
    EXPECT_TRUE(span >= 23 * 3600 && span <= 25 * 3600);
}

TEST_CASE(calendar_weekText) {
    // 0 表示星期日
    EXPECT_STREQ(TimeUtils::getDayOnWeek(0), "星期日");
    EXPECT_STREQ(TimeUtils::getDayOnWeek(6), "星期六");
    // 越界索引按模 7 归一化
    EXPECT_STREQ(TimeUtils::getDayOnWeek(7), "星期日");
    EXPECT_STREQ(TimeUtils::getDayOnWeek(2, "周"), "周二");
    // 负数（失败哨兵值）不能伪装成"星期六"
    EXPECT_STREQ(TimeUtils::getDayOnWeek(-1), "");
    EXPECT_STREQ(TimeUtils::getDayOnWeek(-1, "周"), "");
}

TEST_CASE(timefmt_duration) {
    EXPECT_STREQ(TimeUtils::formatDuration(0), "00:00:00");
    EXPECT_STREQ(TimeUtils::formatDuration(61), "00:01:01");
    EXPECT_STREQ(TimeUtils::formatDuration(3600), "01:00:00");
    // 负数按 0 处理
    EXPECT_STREQ(TimeUtils::formatDuration(-5), "00:00:00");
    // 不足一天不显示天数
    EXPECT_STREQ(TimeUtils::formatDuration(90061), "1天 01:01:01");
    // 不显示天数时小时可超过 24
    EXPECT_STREQ(TimeUtils::formatDuration(90061, false), "25:01:01");
}

TEST_CASE(timefmt_format) {
    const time_t stamp = TimeUtils::makeTime(2024, 6, 19, 8, 5, 3);
    EXPECT_STREQ(TimeUtils::getTimeFmtStr(stamp, "%Y-%m-%d %H:%M:%S"),
        "2024-06-19 08:05:03");
    // 空格式串应返回空
    EXPECT_STREQ(TimeUtils::getTimeFmtStr(stamp, ""), "");
    // 上下午分支
    EXPECT_STREQ(TimeUtils::getTimeFmtStrAP(stamp, "AM%H", "PM%H"), "AM08");
}

TEST_CASE(time_nowAndLocal) {
    const time_t now = TimeUtils::getTimeSec();
    EXPECT_TRUE(now > 0);
    EXPECT_TRUE(TimeUtils::getTimeMSec() > 0);

    std::tm tmValue = {};
    EXPECT_TRUE(TimeUtils::localTime(now, tmValue));
    EXPECT_TRUE(tmValue.tm_year > 100);
}
