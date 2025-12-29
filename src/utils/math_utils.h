/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:46
 * @LastEditTime: 2025-12-29 11:20:26
 * @FilePath: /kk_frame/src/utils/math_utils.h
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

    /// @brief 将两个uint8_t类型的数据组合成一个uint16_t类型的数据
    /// @param h
    /// @param l
    /// @return 
    uint8_t toU16(uint8_t h, uint8_t l);

    /// @brief 将四个uint8_t类型的数据组合成一个uint32_t类型的数据
    /// @param h 
    /// @param m 
    /// @param l 
    /// @param b 
    /// @return 
    uint32_t toU32(uint8_t h, uint8_t m, uint8_t l, uint8_t b);

    /// @brief 将rgb值转换为int32_t类型
    /// @param r 
    /// @param g 
    /// @param b 
    /// @return 
    inline int32_t rgb(uint8_t r, uint8_t g, uint8_t b);

    /// @brief 将rgba值转换为int32_t类型
    /// @param r 
    /// @param g 
    /// @param b 
    /// @param a 
    /// @return 
    inline int32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

} // MathUtils

#endif // !__MATH_UTILS_H__
