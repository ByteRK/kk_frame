/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:46
 * @LastEditTime: 2026-11-05 11:57:19
 * @FilePath: /kk_frame/src/utils/base/math_utils.h
 * @Description: 计算相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __MATH_UTILS_H__
#define __MATH_UTILS_H__

#include <cstdint>

namespace MathUtils {

    /// @brief 生成一个随机数（[min, max] 闭区间，min > max 时自动交换）
    /// @param min 最小值
    /// @param max 最大值
    /// @return 随机数
    /// @note 每个线程持有独立的 mt19937 引擎（thread_local），无锁无竞争，线程安全
    int rand(int min, int max);

    /// @brief 将两个uint8_t类型的数据组合成一个uint16_t类型的数据
    /// @param h 高位
    /// @param l 低位
    /// @return 结果
    uint16_t toU16(uint8_t h, uint8_t l);

    /// @brief 将四个uint8_t类型的数据组合成一个uint32_t类型的数据
    /// @param h 高位
    /// @param m 中间位
    /// @param l 低位
    /// @param b 末位
    /// @return 结果
    uint32_t toU32(uint8_t h, uint8_t m, uint8_t l, uint8_t b);

    /// @brief 将rgb值转换为int32_t类型
    /// @param r 红
    /// @param g 绿
    /// @param b 蓝
    /// @return 结果
    int32_t rgb(uint8_t r, uint8_t g, uint8_t b);

    /// @brief 将rgba值转换为int32_t类型
    /// @param r 红
    /// @param g 绿
    /// @param b 蓝
    /// @param a 透明度
    /// @return 结果
    int32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

} // MathUtils

#endif // !__MATH_UTILS_H__
