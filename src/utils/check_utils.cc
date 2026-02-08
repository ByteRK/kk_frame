/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-02-08 11:01:39
 * @FilePath: /kk_frame/src/utils/check_utils.cc
 * @Description: 校验相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "check_utils.h"

uint8_t CheckUtils::xorCheck(const uint8_t* data, uint64_t length) {
    uint8_t result = 0;
    for (uint64_t i = 0; i < length; i++) {
        result ^= data[i];
    }
    return result;
}

uint16_t CheckUtils::crc16CCITT(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0x0000;   //初始值
    uint16_t i, j;
    for (j = 0; j < length; j++) {
        crc ^= data[j];
        for (i = 0; i < 8; i++) {
            if ((crc & 0x0001) > 0) {
                crc = (crc >> 1) ^ 0x8408;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16MODBUS(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xffff;   //初始值
    uint16_t i, j;
    for (j = 0; j < length; j++) {
        crc ^= data[j];
        for (i = 0; i < 8; i++) {
            if ((crc & 0x0001) > 0) {
                crc = (crc >> 1) ^ 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16ANSI(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0x0000;  // 初始值
    uint8_t i, j;
    for (i = 0; i < length; i++) {
        crc ^= data[i];				
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x8408;  // 多项式 0x1021 的位反射						
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16IBM(const uint8_t* data, uint16_t length) {
    return crc16ANSI(data, length);
}
