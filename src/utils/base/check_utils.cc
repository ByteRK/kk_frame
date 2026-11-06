/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-11-06 10:20:00
 * @FilePath: /kk_frame/src/utils/base/check_utils.cc
 * @Description: 校验和 / CRC 相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "check_utils.h"

namespace {

    /// @brief 非反射式 CRC16（poly 固定 0x1021）
    uint16_t crc16Normal(const uint8_t* data, size_t length, uint16_t initialValue) {
        uint16_t crc = initialValue;
        for (size_t i = 0; i < length; i++) {
            crc ^= static_cast<uint16_t>(data[i]) << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
            }
        }
        return crc;
    }

    /// @brief 反射式 CRC16（poly 由调用方给出）
    uint16_t crc16Reflected(const uint8_t* data, size_t length,
        uint16_t initialValue, uint16_t polynomial) {
        uint16_t crc = initialValue;
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                crc = (crc & 0x0001) ? ((crc >> 1) ^ polynomial) : (crc >> 1);
            }
        }
        return crc;
    }

} // namespace

uint8_t CheckUtils::xorCheck(const uint8_t* data, uint64_t length) {
    uint8_t result = 0;
    for (uint64_t i = 0; i < length; i++) {
        result ^= data[i];
    }
    return result;
}

uint8_t CheckUtils::crc8MAXIM(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x01) ? ((crc >> 1) ^ 0x8C) : (crc >> 1);
        }
    }
    return crc;
}

uint8_t CheckUtils::crc8SMBUS(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16CCITT(const uint8_t* data, size_t length) {
    return crc16Normal(data, length, 0xFFFF);
}

uint16_t CheckUtils::crc16XMODEM(const uint8_t* data, size_t length) {
    return crc16Normal(data, length, 0x0000);
}

uint16_t CheckUtils::crc16KERMIT(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0x0000, 0x8408);
}

uint16_t CheckUtils::crc16MODBUS(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0xFFFF, 0xA001);
}

uint16_t CheckUtils::crc16ANSI(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0x0000, 0xA001);
}

uint16_t CheckUtils::crc16IBM(const uint8_t* data, size_t length) {
    return crc16ANSI(data, length);
}

uint32_t CheckUtils::crc32IEEE(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x00000001u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
