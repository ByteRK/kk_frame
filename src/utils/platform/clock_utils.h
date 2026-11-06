/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-04 14:00:00
 * @FilePath: /kk_frame/src/utils/platform/clock_utils.h
 * @Description: 系统时钟相关的一些函数（设置时间、硬件时钟同步、单调时钟）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CLOCK_UTILS_H__
#define __CLOCK_UTILS_H__

#include <cstdint>

/// @brief 系统时钟操作
/// @note 本模块只负责「读写系统时钟」这类需要特权/平台接口的动作；
///       取当前时刻、日历换算与格式化统一用 TimeUtils。
namespace ClockUtils {

    /// @brief 设置系统时间
    /// @param timestamp 秒级时间戳
    /// @return true 已成功写入系统时钟
    /// @note 无权限、平台未适配（PRODUCT_X64）或 settimeofday 失败时返回 false
    bool setTime(const int64_t& timestamp);

    /// @brief 设置系统时间，参数小于 0 表示保持当前值
    /// @param year 年，大于 0 时生效
    /// @param month 月，1-12 时生效
    /// @param day 日，大于 0 时生效
    /// @param hour 时，0-23 时生效
    /// @param minute 分，0-59 时生效
    /// @param second 秒，0-59 时生效
    /// @return true 已成功写入系统时钟；参数非法或写入失败返回 false
    bool setTime(int year, int month, int day, int hour, int minute, int second = 0);

    /// @brief 同步硬件时钟（仅在已适配的平台生效）
    void syncHWClock();

    /// @brief 获取单调时钟毫秒数
    /// @return 单调毫秒数（不随系统校时影响）
    /// @note 用于测量耗时；等价于 cdroid::SystemClock::uptimeMillis 的语义，
    ///       但在不依赖 cdroid 的场景（如 utils 内部）可用本接口
    /// @note CLOCK_MONOTONIC 不可用时退化为墙钟，避免返回 0 导致超时计算失效
    int64_t monotonicMs();

} // ClockUtils

#endif // !__CLOCK_UTILS_H__
