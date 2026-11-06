/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-04 12:00:00
 * @FilePath: /kk_frame/src/utils/image_utils.h
 * @Description: 图像处理相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __IMAGE_UTILS_H__
#define __IMAGE_UTILS_H__

#include <cstdint>

/// @brief 图像处理相关函数（由 library 中的模糊算法库封装而来）
/// @note 都受 PRJ_LIB_ENABLED(...) 控制，对应库未启用时只打印错误日志
namespace ImageUtils {

    /// @brief 高斯模糊（纯 <math.h> 实现 GaussianBlurFilter）
    /// @param input 需要模糊的图像数据（原地修改）
    /// @param width 图像的宽度
    /// @param height 图像的高度
    /// @param sigma 模糊半径
    void gaussianBlur(uint8_t* input, int width, int height, float sigma);

    /// @brief 高斯模糊（Neon 指令集 gaussianFilter_u8_Neon）
    /// @param src 输入图像数据
    /// @param dst 输出图像数据
    /// @param height 图像的高度（注意：本接口 height 在前，与算法库签名一致）
    /// @param width 图像的宽度
    /// @param channel 图像的通道数
    /// @param ksize 模糊半径
    void gaussianBlurNeon(uint8_t* src, uint8_t* dst, int height, int width,
        int channel, int ksize);

    /// @brief 高斯模糊（FastGaussianBlur -> GaussianBlur4）
    /// @param src 输入图像数据
    /// @param dst 输出图像数据
    /// @param width 图像的宽度
    /// @param height 图像的高度
    /// @param channel 图像的通道数
    /// @param radius 模糊半径
    void gaussianBlurFast(uint8_t* src, uint8_t* dst, int width, int height,
        int channel, int radius);

} // ImageUtils

#endif // !__IMAGE_UTILS_H__
