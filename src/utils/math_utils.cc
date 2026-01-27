/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:51
 * @LastEditTime: 2026-01-27 10:58:28
 * @FilePath: /kk_frame/src/utils/math_utils.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "math_utils.h"
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

uint8_t MathUtils::toU16(uint8_t h, uint8_t l) {
    return h << 8 | l;
}

uint32_t MathUtils::toU32(uint8_t h, uint8_t m, uint8_t l, uint8_t b) {
    return h << 24 | m << 16 | l << 8 | b;
}

int32_t MathUtils::rgb(uint8_t r, uint8_t g, uint8_t b) {
    return rgba(r, g, b, 0xFF);
}

int32_t MathUtils::rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return a << 24 | r << 16 | g << 8 | b;
}

void MathUtils::gaussianBlur(uint8_t* input, int width, int height, float sigma) {
#if PRJ_LIB_ENABLED(GAUSSIANBLUR)
    GaussianBlurFilter(input, width, height, sigma);
#else
    LOGE("GAUSSIANBLUR not enabled");
#endif
}

void MathUtils::gaussianBlur2(uint8_t* src, uint8_t* dst, int height, int width, int channel, int ksize) {
#if PRJ_LIB_ENABLED(GAUSSFILTER)
    gaussianFilter_u8_Neon(src, dst, height, width, channel, ksize);
#else
    LOGE("GAUSSFILTER not enabled");
#endif
}

void MathUtils::gaussianBlur3(uint8_t* scl, uint8_t* tcl, int w, int h, int ch, int r) {
#if PRJ_LIB_ENABLED(FASTGAUSSIANBLUR)
    GaussianBlur4(scl, tcl, w, h, ch, r);
#else
    LOGE("FASTGAUSSIANBLUR not enabled");
#endif
}
