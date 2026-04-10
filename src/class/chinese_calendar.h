/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-11 00:51:34
 * @LastEditTime: 2026-04-11 02:07:15
 * @FilePath: /kk_frame/src/class/chinese_calendar.h
 * @Description: 农历+干支+生肖+节气 计算
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CHINESE_CALENDAR_H__
#define __CHINESE_CALENDAR_H__

#include <ctime>
#include <cstdint>
#include <string>

/// @brief 农历+干支+生肖+节气 计算
class ChineseCalendar {
public:
    /// @brief 农历
    struct LunarDate {
        int  year;    // 年
        int  month;   // 月
        int  day;     // 日
        bool isLeap;  // 是否闰月

        std::string leapText() const;
        std::string monthText() const;
        std::string dayText() const;
        
        std::string toString() const;
    };

    /// @brief 干支
    struct Ganzhi {
        std::string year;  // 年柱
        std::string month; // 月柱
        std::string day;   // 日柱
        std::string hour;  // 时柱

        std::string toString() const;
    };

    /// @brief 节气信息
    struct SolarTermInfo {
        std::string current;    // 当前节气
        int         dayIndex;   // 处于当前节气的第几天，从 1 开始
        std::string next;       // 下一个节气
        int         daysToNext; // 距离下一个节气的天数，按日期差计算

        std::string toString() const;
    };

    /// @brief 结果
    struct Result {
        LunarDate     lunar;      // 农历
        Ganzhi        ganzhi;     // 干支
        std::string   zodiac;     // 生肖
        SolarTermInfo solarTerm;  // 节气信息
    };

public:
    static Result        get();
    static Result        get(std::time_t t);
    static Result        get(const std::tm& tmVal);
    static Result        get(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);

    static LunarDate     getLunar();
    static Ganzhi        getGanzhi();
    static std::string   getZodiac();
    static SolarTermInfo getSolarTerm();

    static LunarDate     getLunar(std::time_t t);
    static Ganzhi        getGanzhi(std::time_t t);
    static std::string   getZodiac(std::time_t t);
    static SolarTermInfo getSolarTerm(std::time_t t);

    static LunarDate     getLunar(const std::tm& tmVal);
    static Ganzhi        getGanzhi(const std::tm& tmVal);
    static std::string   getZodiac(const std::tm& tmVal);
    static SolarTermInfo getSolarTerm(const std::tm& tmVal);

    static LunarDate     getLunar(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);
    static Ganzhi        getGanzhi(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);
    static std::string   getZodiac(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);
    static SolarTermInfo getSolarTerm(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);

private:
    struct DateTime {
        int year;    // 年
        int month;   // 月
        int day;     // 日
        int hour;    // 时
        int minute;  // 分
        int second;  // 秒
    };

    enum CalcMask {
        CALC_NONE       = 0,
        CALC_LUNAR      = 1 << 0,  // 农历
        CALC_GANZHI     = 1 << 1,  // 干支
        CALC_ZODIAC     = 1 << 2,  // 生肖
        CALC_SOLAR_TERM = 1 << 3,  // 节气
        CALC_ALL        = CALC_LUNAR | CALC_GANZHI | CALC_ZODIAC | CALC_SOLAR_TERM
    };

private:
    static int         mod(int a, int b);

    static const char* heavenlyStem(int idx);
    static const char* earthlyBranch(int idx);
    static std::string gz(int stemIdx, int branchIdx);

    static bool        isValidDateTime(int year, int month, int day,
        int hour, int minute, int second);

    static bool        isLeapYear(int year);

    static int         daysFromCivil(int year, int month, int day);
    static void        civilFromDays(int z, int& year, int& month, int& day);
    static DateTime    civilFromSeconds(int64_t totalSeconds);

    static DateTime    makeDateTime(int year, int month, int day,
        int hour, int minute, int second);
    static bool        lessThan(const DateTime& a, const DateTime& b);
    static bool        greaterEqual(const DateTime& a, const DateTime& b);

    static std::tm     toLocalTm(std::time_t t);
    static Result      getInternal(int year, int month, int day,
        int hour, int minute, int second, unsigned mask);

    // 农历

    static int         leapMonth(int year);
    static int         leapDays(int year);
    static int         monthDays(int year, int month);
    static int         yearDays(int year);
    static LunarDate   solarToLunar(int year, int month, int day);

    // 生肖

    static std::string zodiacOfLunarYear(int lunarYear);

    // 节气 / 干支

    static const char* solarTermName(int idx);
    static DateTime    solarTermDateTime(int year, int termIndex);
    static SolarTermInfo currentSolarTermInfo(int year, int month, int day,
        int hour, int minute, int second);
    static Ganzhi      calcGanzhi(int year, int month, int day,
        int hour, int minute, int second);

private:
    static const uint32_t lunarInfo[200];
    static const int      sTermInfo[24];
};

#endif // __CHINESE_CALENDAR_H__