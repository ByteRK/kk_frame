/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-04 12:00:00
 * @FilePath: /kk_frame/src/utils/image_utils.cc
 * @Description: 图像处理相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "image_utils.h"

#include <cdlog.h>

#include "library/library_config.h"

#if PRJ_LIB_ENABLED(GAUSSIANBLUR)
#include "gaussianblur.h"
#endif
#if PRJ_LIB_ENABLED(GAUSSFILTER)
#include "gaussFilter.h"
#endif
#if PRJ_LIB_ENABLED(FASTGAUSSIANBLUR)
#include "blur.h"
#endif

void ImageUtils::gaussianBlur(uint8_t* input, int width, int height, float sigma) {
#if PRJ_LIB_ENABLED(GAUSSIANBLUR)
    GaussianBlurFilter(input, width, height, sigma);
#else
    LOGE("GAUSSIANBLUR not enabled");
#endif
}

void ImageUtils::gaussianBlurNeon(uint8_t* src, uint8_t* dst, int height, int width,
    int channel, int ksize) {
#if PRJ_LIB_ENABLED(GAUSSFILTER)
    gaussianFilter_u8_Neon(src, dst, height, width, channel, ksize);
#else
    LOGE("GAUSSFILTER not enabled");
#endif
}

void ImageUtils::gaussianBlurFast(uint8_t* src, uint8_t* dst, int width, int height,
    int channel, int radius) {
#if PRJ_LIB_ENABLED(FASTGAUSSIANBLUR)
    GaussianBlur4(src, dst, width, height, channel, radius);
#else
    LOGE("FASTGAUSSIANBLUR not enabled");
#endif
}
