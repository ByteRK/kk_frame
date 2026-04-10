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

#include "chinese_calendar.h"

#include <sstream>
#include <stdexcept>

/**
 * @brief 获取闰月前缀文本
 *
 * @return 当前为闰月时返回“闰”，否则返回空字符串
 */
std::string ChineseCalendar::LunarDate::leapText() const {
    return isLeap ? "闰" : "";
}

/**
 * @brief 获取农历月份中文文本
 *
 * @return 形如“正”“二”“三”“冬”“腊”的月份文本
 */
std::string ChineseCalendar::LunarDate::monthText() const {
    static const char* kMonthName[] = {
        "", "正", "二", "三", "四", "五", "六",
        "七", "八", "九", "十", "冬", "腊"
    };
    if (month < 1 || month > 12) {
        return "";
    }
    return kMonthName[month];
}

/**
 * @brief 获取农历日期中文文本
 *
 * @return 形如“初一”“十五”“廿九”的日期文本
 */
std::string ChineseCalendar::LunarDate::dayText() const {
    static const char* kDayName[] = {
        "",
        "初一","初二","初三","初四","初五","初六","初七","初八","初九","初十",
        "十一","十二","十三","十四","十五","十六","十七","十八","十九","二十",
        "廿一","廿二","廿三","廿四","廿五","廿六","廿七","廿八","廿九","三十"
    };
    if (day < 1 || day > 30) {
        return "";
    }
    return kDayName[day];
}

/**
 * @brief 将农历日期转换为完整中文字符串
 *
 * @return 形如“闰二月初三”或“正月十五”的完整农历文本
 */
std::string ChineseCalendar::LunarDate::toString() const {
    std::ostringstream oss;
    oss << leapText() << monthText() << "月" << dayText();
    return oss.str();
}

/**
 * @brief 将干支结果拼接为便于日志输出的字符串
 *
 * @return 形如“年柱=甲辰 月柱=丙寅 日柱=甲子 时柱=丙子”的文本
 */
std::string ChineseCalendar::Ganzhi::toString() const {
    std::ostringstream oss;
    oss << "年柱=" << year
        << " 月柱=" << month
        << " 日柱=" << day
        << " 时柱=" << hour;
    return oss.str();
}

/**
 * @brief 将节气信息拼接为便于日志输出的字符串
 *
 * @return 形如“当前节气=清明 第3天 下一个节气=谷雨 距离12天”的文本
 */
std::string ChineseCalendar::SolarTermInfo::toString() const {
    std::ostringstream oss;
    oss << "当前节气=" << current
        << " 第" << dayIndex << "天"
        << " 下一个节气=" << next
        << " 距离" << daysToNext << "天";
    return oss.str();
}

/**
 * @brief 获取当前本地时间对应的完整历法结果
 *
 * @return 当前时刻的农历、干支、生肖和节气信息
 */
ChineseCalendar::Result ChineseCalendar::get() {
    std::time_t t = std::time(NULL);
    return get(t);
}

/**
 * @brief 根据 time_t 获取完整历法结果
 *
 * @param t Unix 时间戳，按本地时区解释
 * @return 指定时间对应的农历、干支、生肖和节气信息
 */
ChineseCalendar::Result ChineseCalendar::get(std::time_t t) {
    std::tm localTm = toLocalTm(t);
    return get(localTm);
}

/**
 * @brief 根据 std::tm 获取完整历法结果
 *
 * @param tmVal 本地时间结构体
 * @return 指定时间对应的农历、干支、生肖和节气信息
 */
ChineseCalendar::Result ChineseCalendar::get(const std::tm& tmVal) {
    return get(
        tmVal.tm_year + 1900,
        tmVal.tm_mon + 1,
        tmVal.tm_mday,
        tmVal.tm_hour,
        tmVal.tm_min,
        tmVal.tm_sec
    );
}

/**
 * @brief 根据年月日时分秒获取完整历法结果
 *
 * @param year 公历年
 * @param month 公历月，范围 [1, 12]
 * @param day 公历日
 * @param hour 小时，范围 [0, 23]
 * @param minute 分钟，范围 [0, 59]
 * @param second 秒，范围 [0, 59]
 * @return 指定时间对应的完整历法结果
 */
ChineseCalendar::Result ChineseCalendar::get(int year, int month, int day,
    int hour, int minute, int second) {
    return getInternal(year, month, day, hour, minute, second, CALC_ALL);
}

/**
 * @brief 获取当前本地时间对应的农历
 *
 * @return 当前时刻的农历信息
 */
ChineseCalendar::LunarDate ChineseCalendar::getLunar() {
    std::time_t t = std::time(NULL);
    return getLunar(t);
}

/**
 * @brief 获取当前本地时间对应的干支
 *
 * @return 当前时刻的干支信息
 */
ChineseCalendar::Ganzhi ChineseCalendar::getGanzhi() {
    std::time_t t = std::time(NULL);
    return getGanzhi(t);
}

/**
 * @brief 获取当前本地时间对应的生肖
 *
 * @return 当前时刻对应的生肖
 */
std::string ChineseCalendar::getZodiac() {
    std::time_t t = std::time(NULL);
    return getZodiac(t);
}

/**
 * @brief 获取当前本地时间对应的节气信息
 *
 * @return 当前时刻对应的节气信息
 */
ChineseCalendar::SolarTermInfo ChineseCalendar::getSolarTerm() {
    std::time_t t = std::time(NULL);
    return getSolarTerm(t);
}

/**
 * @brief 根据 time_t 获取农历
 *
 * @param t Unix 时间戳，按本地时区解释
 * @return 指定时间对应的农历信息
 */
ChineseCalendar::LunarDate ChineseCalendar::getLunar(std::time_t t) {
    std::tm localTm = toLocalTm(t);
    return getLunar(localTm);
}

/**
 * @brief 根据 time_t 获取干支
 *
 * @param t Unix 时间戳，按本地时区解释
 * @return 指定时间对应的干支信息
 */
ChineseCalendar::Ganzhi ChineseCalendar::getGanzhi(std::time_t t) {
    std::tm localTm = toLocalTm(t);
    return getGanzhi(localTm);
}

/**
 * @brief 根据 time_t 获取生肖
 *
 * @param t Unix 时间戳，按本地时区解释
 * @return 指定时间对应的生肖
 */
std::string ChineseCalendar::getZodiac(std::time_t t) {
    std::tm localTm = toLocalTm(t);
    return getZodiac(localTm);
}

/**
 * @brief 根据 time_t 获取节气信息
 *
 * @param t Unix 时间戳，按本地时区解释
 * @return 指定时间对应的节气信息
 */
ChineseCalendar::SolarTermInfo ChineseCalendar::getSolarTerm(std::time_t t) {
    std::tm localTm = toLocalTm(t);
    return getSolarTerm(localTm);
}

/**
 * @brief 根据 std::tm 获取农历
 *
 * @param tmVal 本地时间结构体
 * @return 指定时间对应的农历信息
 */
ChineseCalendar::LunarDate ChineseCalendar::getLunar(const std::tm& tmVal) {
    return getLunar(
        tmVal.tm_year + 1900,
        tmVal.tm_mon + 1,
        tmVal.tm_mday,
        tmVal.tm_hour,
        tmVal.tm_min,
        tmVal.tm_sec
    );
}

/**
 * @brief 根据 std::tm 获取干支
 *
 * @param tmVal 本地时间结构体
 * @return 指定时间对应的干支信息
 */
ChineseCalendar::Ganzhi ChineseCalendar::getGanzhi(const std::tm& tmVal) {
    return getGanzhi(
        tmVal.tm_year + 1900,
        tmVal.tm_mon + 1,
        tmVal.tm_mday,
        tmVal.tm_hour,
        tmVal.tm_min,
        tmVal.tm_sec
    );
}

/**
 * @brief 根据 std::tm 获取生肖
 *
 * @param tmVal 本地时间结构体
 * @return 指定时间对应的生肖
 */
std::string ChineseCalendar::getZodiac(const std::tm& tmVal) {
    return getZodiac(
        tmVal.tm_year + 1900,
        tmVal.tm_mon + 1,
        tmVal.tm_mday,
        tmVal.tm_hour,
        tmVal.tm_min,
        tmVal.tm_sec
    );
}

/**
 * @brief 根据 std::tm 获取节气信息
 *
 * @param tmVal 本地时间结构体
 * @return 指定时间对应的节气信息
 */
ChineseCalendar::SolarTermInfo ChineseCalendar::getSolarTerm(const std::tm& tmVal) {
    return getSolarTerm(
        tmVal.tm_year + 1900,
        tmVal.tm_mon + 1,
        tmVal.tm_mday,
        tmVal.tm_hour,
        tmVal.tm_min,
        tmVal.tm_sec
    );
}

/**
 * @brief 根据年月日时分秒获取农历
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 指定时间对应的农历信息
 */
ChineseCalendar::LunarDate ChineseCalendar::getLunar(int year, int month, int day,
    int hour, int minute, int second) {
    return getInternal(year, month, day, hour, minute, second, CALC_LUNAR).lunar;
}

/**
 * @brief 根据年月日时分秒获取干支
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 指定时间对应的干支信息
 */
ChineseCalendar::Ganzhi ChineseCalendar::getGanzhi(int year, int month, int day,
    int hour, int minute, int second) {
    return getInternal(year, month, day, hour, minute, second, CALC_GANZHI).ganzhi;
}

/**
 * @brief 根据年月日时分秒获取生肖
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 指定时间对应的生肖
 */
std::string ChineseCalendar::getZodiac(int year, int month, int day,
    int hour, int minute, int second) {
    return getInternal(year, month, day, hour, minute, second, CALC_ZODIAC).zodiac;
}

/**
 * @brief 根据年月日时分秒获取节气信息
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 指定时间对应的节气信息
 */
ChineseCalendar::SolarTermInfo ChineseCalendar::getSolarTerm(int year, int month, int day,
    int hour, int minute, int second) {
    return getInternal(year, month, day, hour, minute, second, CALC_SOLAR_TERM).solarTerm;
}

/**
 * @brief 计算数学意义下的正模值
 *
 * @param a 被除数
 * @param b 除数
 * @return 始终非负的取模结果
 */
int ChineseCalendar::mod(int a, int b) {
    int r = a % b;
    return (r < 0) ? (r + b) : r;
}

/**
 * @brief 根据序号返回天干字符
 *
 * @param idx 天干索引
 * @return 对应的天干字符串
 */
const char* ChineseCalendar::heavenlyStem(int idx) {
    static const char* kStem[10] = {
        "甲","乙","丙","丁","戊","己","庚","辛","壬","癸"
    };
    return kStem[mod(idx, 10)];
}

/**
 * @brief 根据序号返回地支字符
 *
 * @param idx 地支索引
 * @return 对应的地支字符串
 */
const char* ChineseCalendar::earthlyBranch(int idx) {
    static const char* kBranch[12] = {
        "子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"
    };
    return kBranch[mod(idx, 12)];
}

/**
 * @brief 拼接天干和地支为完整干支文本
 *
 * @param stemIdx 天干索引
 * @param branchIdx 地支索引
 * @return 干支字符串，例如“甲子”
 */
std::string ChineseCalendar::gz(int stemIdx, int branchIdx) {
    return std::string(heavenlyStem(stemIdx)) + earthlyBranch(branchIdx);
}

/**
 * @brief 校验日期时间是否合法
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 合法返回 true，否则返回 false
 */
bool ChineseCalendar::isValidDateTime(int year, int month, int day,
    int hour, int minute, int second) {
    if (year < 1) return false;
    if (month < 1 || month > 12) return false;
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;

    static const int kDaysInMonth[] = {
        0, 31,28,31,30,31,30,31,31,30,31,30,31
    };

    int maxDay = kDaysInMonth[month];
    if (month == 2 && isLeapYear(year)) {
        maxDay = 29;
    }
    if (day < 1 || day > maxDay) return false;

    return true;
}

/**
 * @brief 判断公历年份是否为闰年
 *
 * @param year 公历年
 * @return 闰年返回 true，否则返回 false
 */
bool ChineseCalendar::isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

/**
 * @brief 将公历日期转换为内部连续天数
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @return 对应的连续天数值
 */
int ChineseCalendar::daysFromCivil(int year, int month, int day) {
    year -= (month <= 2);
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 60;
}

/**
 * @brief 将内部连续天数还原为公历年月日
 *
 * @param z 连续天数值
 * @param year 输出公历年
 * @param month 输出公历月
 * @param day 输出公历日
 */
void ChineseCalendar::civilFromDays(int z, int& year, int& month, int& day) {
    z += 60;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    month = static_cast<int>(mp + (mp < 10 ? 3 : -9));
    year += (month <= 2);
}

/**
 * @brief 将秒数偏移还原为日期时间
 *
 * @param totalSeconds 自内部基准起算的秒数
 * @return 还原后的日期时间结构
 */
ChineseCalendar::DateTime ChineseCalendar::civilFromSeconds(int64_t totalSeconds) {
    int64_t days = totalSeconds / 86400LL;
    int64_t secOfDay = totalSeconds % 86400LL;
    if (secOfDay < 0) {
        secOfDay += 86400LL;
        --days;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    civilFromDays(static_cast<int>(days), year, month, day);

    DateTime dt;
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = static_cast<int>(secOfDay / 3600LL);
    dt.minute = static_cast<int>((secOfDay % 3600LL) / 60LL);
    dt.second = static_cast<int>(secOfDay % 60LL);
    return dt;
}

/**
 * @brief 构造内部日期时间结构
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 组装后的日期时间结构
 */
ChineseCalendar::DateTime ChineseCalendar::makeDateTime(int year, int month, int day,
    int hour, int minute, int second) {
    DateTime dt;
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = hour;
    dt.minute = minute;
    dt.second = second;
    return dt;
}

/**
 * @brief 判断日期时间 a 是否早于 b
 *
 * @param a 左值
 * @param b 右值
 * @return a 早于 b 返回 true，否则返回 false
 */
bool ChineseCalendar::lessThan(const DateTime& a, const DateTime& b) {
    if (a.year != b.year) return a.year < b.year;
    if (a.month != b.month) return a.month < b.month;
    if (a.day != b.day) return a.day < b.day;
    if (a.hour != b.hour) return a.hour < b.hour;
    if (a.minute != b.minute) return a.minute < b.minute;
    return a.second < b.second;
}

/**
 * @brief 判断日期时间 a 是否晚于或等于 b
 *
 * @param a 左值
 * @param b 右值
 * @return a 晚于或等于 b 返回 true，否则返回 false
 */
bool ChineseCalendar::greaterEqual(const DateTime& a, const DateTime& b) {
    return !lessThan(a, b);
}

/**
 * @brief 将 Unix 时间戳转换为本地时间结构体
 *
 * @param t Unix 时间戳
 * @return 本地时间结构体
 */
std::tm ChineseCalendar::toLocalTm(std::time_t t) {
    std::tm localTm;
#if defined(_WIN32)
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif
    return localTm;
}

/**
 * @brief 内部统一计算入口，按位掩码控制实际计算项
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @param mask 计算项掩码
 * @return 仅填充所请求字段后的结果结构
 */
ChineseCalendar::Result ChineseCalendar::getInternal(int year, int month, int day,
    int hour, int minute, int second, unsigned mask) {
    if (!isValidDateTime(year, month, day, hour, minute, second)) {
        throw std::invalid_argument("invalid datetime");
    }

    if (year < 1900 || year > 2099) {
        throw std::out_of_range("ChineseCalendar supports years in [1900, 2099]");
    }

    Result r;
    r.lunar.year = 0;
    r.lunar.month = 0;
    r.lunar.day = 0;
    r.lunar.isLeap = false;
    r.zodiac.clear();
    r.solarTerm.current.clear();
    r.solarTerm.dayIndex = 0;
    r.solarTerm.next.clear();
    r.solarTerm.daysToNext = 0;

    const bool needLunar = (mask & (CALC_LUNAR | CALC_ZODIAC)) != 0;

    if (needLunar) {
        r.lunar = solarToLunar(year, month, day);
    }
    if (mask & CALC_ZODIAC) {
        r.zodiac = zodiacOfLunarYear(r.lunar.year);
    }
    if (mask & CALC_GANZHI) {
        r.ganzhi = calcGanzhi(year, month, day, hour, minute, second);
    }
    if (mask & CALC_SOLAR_TERM) {
        r.solarTerm = currentSolarTermInfo(year, month, day, hour, minute, second);
    }

    return r;
}

/**
 * @brief 获取指定农历年的闰月月份
 *
 * @param year 农历年
 * @return 无闰月返回 0，否则返回闰月月份
 */
int ChineseCalendar::leapMonth(int year) {
    return lunarInfo[year - 1900] & 0xF;
}

/**
 * @brief 获取指定农历年闰月的天数
 *
 * @param year 农历年
 * @return 闰月天数，无闰月返回 0
 */
int ChineseCalendar::leapDays(int year) {
    if (leapMonth(year)) {
        return (lunarInfo[year - 1900] & 0x10000) ? 30 : 29;
    }
    return 0;
}

/**
 * @brief 获取指定农历年某月的天数
 *
 * @param year 农历年
 * @param month 农历月
 * @return 该月天数，29 或 30
 */
int ChineseCalendar::monthDays(int year, int month) {
    return (lunarInfo[year - 1900] & (0x10000 >> month)) ? 30 : 29;
}

/**
 * @brief 获取指定农历年的总天数
 *
 * @param year 农历年
 * @return 农历年总天数
 */
int ChineseCalendar::yearDays(int year) {
    int sum = 348;
    uint32_t info = lunarInfo[year - 1900];
    for (uint32_t mask = 0x8000; mask > 0x8; mask >>= 1) {
        if (info & mask) {
            ++sum;
        }
    }
    return sum + leapDays(year);
}

/**
 * @brief 将公历日期转换为农历日期
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @return 对应的农历日期
 */
ChineseCalendar::LunarDate ChineseCalendar::solarToLunar(int year, int month, int day) {
    const int baseDays = daysFromCivil(1900, 1, 31);
    int offset = daysFromCivil(year, month, day) - baseDays;

    LunarDate lunar;
    lunar.year = 1900;
    lunar.month = 1;
    lunar.day = 1;
    lunar.isLeap = false;

    int lunarYear = 1900;
    while (lunarYear < 2100 && offset >= yearDays(lunarYear)) {
        offset -= yearDays(lunarYear);
        ++lunarYear;
    }
    lunar.year = lunarYear;

    int leap = leapMonth(lunarYear);
    bool isLeap = false;
    int lunarMonth = 1;

    while (lunarMonth <= 12) {
        int daysInMonth = 0;

        if (leap > 0 && lunarMonth == leap + 1 && !isLeap) {
            --lunarMonth;
            isLeap = true;
            daysInMonth = leapDays(lunarYear);
        } else {
            daysInMonth = monthDays(lunarYear, lunarMonth);
        }

        if (offset < daysInMonth) {
            break;
        }

        offset -= daysInMonth;

        if (isLeap && lunarMonth == leap) {
            isLeap = false;
        }

        ++lunarMonth;
    }

    lunar.month = lunarMonth;
    lunar.day = offset + 1;
    lunar.isLeap = isLeap;
    return lunar;
}

/**
 * @brief 根据农历年份计算生肖
 *
 * @param lunarYear 农历年份
 * @return 对应的生肖文本
 */
std::string ChineseCalendar::zodiacOfLunarYear(int lunarYear) {
    static const char* kZodiac[12] = {
        "鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"
    };
    return kZodiac[mod(lunarYear - 1900, 12)];
}

/**
 * @brief 根据节气索引返回节气名称
 *
 * @param idx 节气索引，范围 [0, 23]
 * @return 节气名称
 */
const char* ChineseCalendar::solarTermName(int idx) {
    static const char* kTerm[24] = {
        "小寒","大寒","立春","雨水","惊蛰","春分",
        "清明","谷雨","立夏","小满","芒种","夏至",
        "小暑","大暑","立秋","处暑","白露","秋分",
        "寒露","霜降","立冬","小雪","大雪","冬至"
    };
    return kTerm[idx];
}

/**
 * @brief 计算指定年份某个节气的大致发生时间
 *
 * @param year 公历年
 * @param termIndex 节气索引，范围 [0, 23]
 * @return 节气对应的日期时间
 */
ChineseCalendar::DateTime ChineseCalendar::solarTermDateTime(int year, int termIndex) {
    const int64_t minutes =
        static_cast<int64_t>(525948.76 * (year - 1900)) +
        static_cast<int64_t>(sTermInfo[termIndex]);

    // 基准：1900-01-06 10:05（按东八区使用）
    const int64_t baseDays = daysFromCivil(1900, 1, 6);
    const int64_t baseSeconds = baseDays * 86400LL + (10 * 3600 + 5 * 60);

    const int64_t totalSeconds = baseSeconds + minutes * 60LL;
    return civilFromSeconds(totalSeconds);
}

/**
 * @brief 获取指定时间对应的节气详细信息
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 包含当前节气、节气第几天、下一个节气和距离天数的结构体
 */
ChineseCalendar::SolarTermInfo ChineseCalendar::currentSolarTermInfo(int year, int month, int day,
    int hour, int minute, int second) {
    const DateTime now = makeDateTime(year, month, day, hour, minute, second);
    SolarTermInfo info;
    info.current.clear();
    info.dayIndex = 0;
    info.next.clear();
    info.daysToNext = 0;

    int currentIdx = -1;
    int nextIdx = -1;
    DateTime currentDt;
    DateTime nextDt;

    const DateTime firstTermOfYear = solarTermDateTime(year, 0);

    if (lessThan(now, firstTermOfYear)) {
        currentIdx = 23;
        nextIdx = 0;
        currentDt = solarTermDateTime(year - 1, 23);
        nextDt = firstTermOfYear;
    } else {
        currentIdx = 0;
        currentDt = firstTermOfYear;

        for (int i = 1; i < 24; ++i) {
            DateTime dt = solarTermDateTime(year, i);
            if (greaterEqual(now, dt)) {
                currentIdx = i;
                currentDt = dt;
            } else {
                break;
            }
        }

        if (currentIdx == 23) {
            nextIdx = 0;
            nextDt = solarTermDateTime(year + 1, 0);
        } else {
            nextIdx = currentIdx + 1;
            nextDt = solarTermDateTime(year, nextIdx);
        }
    }

    const int nowDays = daysFromCivil(year, month, day);
    const int currentDays = daysFromCivil(currentDt.year, currentDt.month, currentDt.day);
    const int nextDays = daysFromCivil(nextDt.year, nextDt.month, nextDt.day);

    info.current = solarTermName(currentIdx);
    info.dayIndex = nowDays - currentDays + 1;
    info.next = solarTermName(nextIdx);
    info.daysToNext = nextDays - nowDays;

    if (info.dayIndex < 1) {
        info.dayIndex = 1;
    }
    if (info.daysToNext < 0) {
        info.daysToNext = 0;
    }

    return info;
}

/**
 * @brief 计算指定时间对应的年柱、月柱、日柱和时柱
 *
 * @param year 公历年
 * @param month 公历月
 * @param day 公历日
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @return 对应的干支结果
 */
ChineseCalendar::Ganzhi ChineseCalendar::calcGanzhi(int year, int month, int day,
    int hour, int minute, int second) {
    Ganzhi out;
    const DateTime now = makeDateTime(year, month, day, hour, minute, second);

    // 年柱：以立春为界
    const DateTime lichun = solarTermDateTime(year, 2);
    int gzYear = lessThan(now, lichun) ? (year - 1) : year;

    // 1984 甲子年
    const int yearIndex60 = mod(gzYear - 1984, 60);
    const int yearStemIdx = mod(yearIndex60, 10);
    const int yearBranchIdx = mod(yearIndex60, 12);
    out.year = gz(yearStemIdx, yearBranchIdx);

    // 月柱：以节为界
    int monthOrder = 0;
    int gzYearForMonth = gzYear;

    const DateTime xiaohan = solarTermDateTime(year, 0);
    const DateTime jingzhe = solarTermDateTime(year, 4);
    const DateTime qingming = solarTermDateTime(year, 6);
    const DateTime lixia = solarTermDateTime(year, 8);
    const DateTime mangzhong = solarTermDateTime(year, 10);
    const DateTime xiaoshu = solarTermDateTime(year, 12);
    const DateTime liqiu = solarTermDateTime(year, 14);
    const DateTime bailu = solarTermDateTime(year, 16);
    const DateTime hanlu = solarTermDateTime(year, 18);
    const DateTime lidong = solarTermDateTime(year, 20);
    const DateTime daxue = solarTermDateTime(year, 22);

    if (lessThan(now, lichun)) {
        gzYearForMonth = year - 1;
        if (lessThan(now, xiaohan)) {
            monthOrder = 11; // 子月
        } else {
            monthOrder = 12; // 丑月
        }
    } else if (lessThan(now, jingzhe)) {
        monthOrder = 1;      // 寅月
    } else if (lessThan(now, qingming)) {
        monthOrder = 2;      // 卯月
    } else if (lessThan(now, lixia)) {
        monthOrder = 3;      // 辰月
    } else if (lessThan(now, mangzhong)) {
        monthOrder = 4;      // 巳月
    } else if (lessThan(now, xiaoshu)) {
        monthOrder = 5;      // 午月
    } else if (lessThan(now, liqiu)) {
        monthOrder = 6;      // 未月
    } else if (lessThan(now, bailu)) {
        monthOrder = 7;      // 申月
    } else if (lessThan(now, hanlu)) {
        monthOrder = 8;      // 酉月
    } else if (lessThan(now, lidong)) {
        monthOrder = 9;      // 戌月
    } else if (lessThan(now, daxue)) {
        monthOrder = 10;     // 亥月
    } else {
        monthOrder = 11;     // 子月
    }

    const int gzYearIndexForMonth = mod(gzYearForMonth - 1984, 60);
    const int yearStemForMonth = mod(gzYearIndexForMonth, 10);
    const int firstMonthStem = mod((yearStemForMonth % 5) * 2 + 2, 10);
    const int monthStemIdx = mod(firstMonthStem + (monthOrder - 1), 10);
    const int monthBranchIdx = mod(monthOrder + 1, 12);
    out.month = gz(monthStemIdx, monthBranchIdx);

    // 日柱：1912-02-18 为甲子日
    const int baseDay = daysFromCivil(1912, 2, 18);
    const int curDay = daysFromCivil(year, month, day);
    const int dayIndex60 = mod(curDay - baseDay, 60);
    const int dayStemIdx = mod(dayIndex60, 10);
    const int dayBranchIdx = mod(dayIndex60, 12);
    out.day = gz(dayStemIdx, dayBranchIdx);

    // 时柱
    const int hourBranchOrder = mod((hour + 1) / 2, 12);
    const int hourBranchIdx = hourBranchOrder;
    const int hourStartStem = (dayStemIdx % 5) * 2;
    const int hourStemIdx = mod(hourStartStem + hourBranchOrder, 10);
    out.hour = gz(hourStemIdx, hourBranchIdx);

    return out;
}

const uint32_t ChineseCalendar::lunarInfo[200] = {
    0x04BD8,0x04AE0,0x0A570,0x054D5,0x0D260,0x0D950,0x16554,0x056A0,0x09AD0,0x055D2,
    0x04AE0,0x0A5B6,0x0A4D0,0x0D250,0x1D255,0x0B540,0x0D6A0,0x0ADA2,0x095B0,0x14977,
    0x04970,0x0A4B0,0x0B4B5,0x06A50,0x06D40,0x1AB54,0x02B60,0x09570,0x052F2,0x04970,
    0x06566,0x0D4A0,0x0EA50,0x06E95,0x05AD0,0x02B60,0x186E3,0x092E0,0x1C8D7,0x0C950,
    0x0D4A0,0x1D8A6,0x0B550,0x056A0,0x1A5B4,0x025D0,0x092D0,0x0D2B2,0x0A950,0x0B557,
    0x06CA0,0x0B550,0x15355,0x04DA0,0x0A5D0,0x14573,0x052D0,0x0A9A8,0x0E950,0x06AA0,
    0x0AEA6,0x0AB50,0x04B60,0x0AAE4,0x0A570,0x05260,0x0F263,0x0D950,0x05B57,0x056A0,
    0x096D0,0x04DD5,0x04AD0,0x0A4D0,0x0D4D4,0x0D250,0x0D558,0x0B540,0x0B5A0,0x195A6,
    0x095B0,0x049B0,0x0A974,0x0A4B0,0x0B27A,0x06A50,0x06D40,0x0AF46,0x0AB60,0x09570,
    0x04AF5,0x04970,0x064B0,0x074A3,0x0EA50,0x06B58,0x05AC0,0x0AB60,0x096D5,0x092E0,

    0x0C960,0x0D954,0x0D4A0,0x0DA50,0x07552,0x056A0,0x0ABB7,0x025D0,0x092D0,0x0CAB5,
    0x0A950,0x0B4A0,0x0BAA4,0x0AD50,0x055D9,0x04BA0,0x0A5B0,0x15176,0x052B0,0x0A930,
    0x07954,0x06AA0,0x0AD50,0x05B52,0x04B60,0x0A6E6,0x0A4E0,0x0D260,0x0EA65,0x0D530,
    0x05AA0,0x076A3,0x096D0,0x04BD7,0x04AD0,0x0A4D0,0x1D0B6,0x0D250,0x0D520,0x0DD45,
    0x0B5A0,0x056D0,0x055B2,0x049B0,0x0A577,0x0A4B0,0x0AA50,0x1B255,0x06D20,0x0ADA0,
    0x14B63,0x09370,0x049F8,0x04970,0x064B0,0x168A6,0x0EA50,0x06AA0,0x1A6C4,0x0AAE0,
    0x0A2E0,0x0D2E3,0x0C960,0x0D557,0x0D4A0,0x0DA50,0x05D55,0x056A0,0x0A6D0,0x055D4,
    0x052D0,0x0A9B8,0x0A950,0x0B4A0,0x0B6A6,0x0AD50,0x055A0,0x0ABA4,0x0A5B0,0x052B0,
    0x0B273,0x06930,0x07337,0x06AA0,0x0AD50,0x14B55,0x04B60,0x0A570,0x054E4,0x0D160,
    0x0E968,0x0D520,0x0DAA0,0x16AA6,0x056D0,0x04AE0,0x0A9D4,0x0A2D0,0x0D150,0x0F252
};

const int ChineseCalendar::sTermInfo[24] = {
    0, 21208, 42467, 63836, 85337, 107014, 128867, 150921,
    173149, 195551, 218072, 240693, 263343, 285989, 308563, 331033,
    353350, 375494, 397447, 419210, 440795, 462224, 483532, 504758
};