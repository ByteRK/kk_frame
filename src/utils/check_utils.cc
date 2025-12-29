/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2025-12-29 11:54:18
 * @FilePath: /kk_frame/src/utils/check_utils.cc
 * @Description: 校验相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "check_utils.h"

uint64_t CheckUtils::checkSum(const char* data, uint64_t length) {
    uint64_t sum = 0;
    for (int i = 0; i < length; i++)
        sum += data[i];
    return sum;
}