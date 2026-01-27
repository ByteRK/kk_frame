/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:46
 * @LastEditTime: 2026-01-27 13:53:47
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

    /// @brief 高斯模糊1
    /// @param input 需要模糊的图像数据
    /// @param width 图像的宽度
    /// @param height 图像的高度
    /// @param sigma 模糊半径
    /// @note 纯<math.h>库进行模糊 GaussianBlurFilter
    void gaussianBlur(uint8_t* input, int width, int height, float sigma);

    /// @brief 高斯模糊2
    /// @param src 输入图像数据
    /// @param dst 输出图像数据
    /// @param height 图像的高度
    /// @param width 图像的宽度
    /// @param channel 图像的通道数
    /// @param ksize 模糊半径
    /// @note 使用Neon的指令集进行模糊 gaussianFilter_u8_Neon
    void gaussianBlur2(uint8_t* src, uint8_t* dst, int height, int width, int channel, int ksize);

    /// @brief 高斯模糊3
    /// @param scl 输入图像数据
    /// @param tcl 输出图像数据
    /// @param w 图像的宽度
    /// @param h 图像的高度
    /// @param ch 图像的通道数
    /// @param r 模糊半径
    /// @note 使用GLM进行模糊 FastGaussianBlur -> GaussianBlur4
    void gaussianBlur3(uint8_t *scl, uint8_t *tcl, int w, int h, int ch, int r);

} // MathUtils

#endif // !__MATH_UTILS_H__
