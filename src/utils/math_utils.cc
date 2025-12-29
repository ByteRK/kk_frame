/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:51
 * @LastEditTime: 2025-12-29 11:20:29
 * @FilePath: /kk_frame/src/utils/math_utils.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "math_utils.h"

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
