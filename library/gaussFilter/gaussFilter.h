/*
 * @Author: cy
 * @Email: patrickcchan@163.com
 * @Date: 2025-11-25 17:24:28
 * @LastEditTime: 2026-01-27 09:08:56
 * @FilePath: /kk_frame/library/gaussFilter/gaussFilter.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
*/

#ifndef _GAUSSFILTER_H_
#define _GAUSSFILTER_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef unsigned char U8;

/**
 * @brief 核分离高斯滤波定点型NEON实现
 * @param src
 * @param dst
 * @param height
 * @param width
 * @param channel
 * @param ksize
 * @param sigma
 */
void gaussianFilter_u8_Neon(U8* src, U8* dst, int height, int width, int channel, int ksize);

#ifdef __cplusplus
}
#endif

#endif //_GAUSSFILTER_H_
