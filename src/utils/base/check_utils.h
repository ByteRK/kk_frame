/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-11-06 09:35:35
 * @FilePath: /kk_frame/src/utils/base/check_utils.h
 * @Description: 校验和 / CRC 相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CHECK_UTILS_H__
#define __CHECK_UTILS_H__

#include <cstddef>
#include <cstdint>

/// @brief 轻量校验：累加和、异或校验、CRC
namespace CheckUtils {

    /// @brief 计算累加和
    /// @tparam T 数据类型
    /// @param data 数据
    /// @param length 数据长度（元素个数）
    /// @return 累加和
    template <typename T>
    uint64_t checkSum(const T* data, size_t length) {
        uint64_t sum = 0;
        for (size_t i = 0; i < length; i++)
            sum += data[i];
        return sum;
    }

    /// @brief 计算异或校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return 异或校验值
    uint8_t xorCheck(const uint8_t* data, uint64_t length);

    /// @brief 计算CRC-8/MAXIM-DOW校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC8校验值
    /// @note poly=0x31, init=0x00, refin=true, refout=true, xorout=0x00
    uint8_t crc8MAXIM(const uint8_t* data, size_t length);

    /// @brief 计算CRC-8/SMBUS校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC8校验值
    /// @note poly=0x07, init=0x00, refin=false, refout=false, xorout=0x00
    uint8_t crc8SMBUS(const uint8_t* data, size_t length);

    /// @brief 计算CRC-16/CCITT-FALSE校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note poly=0x1021, init=0xFFFF, refin=false, refout=false, xorout=0x0000
    uint16_t crc16CCITT(const uint8_t* data, size_t length);

    /// @brief 计算CRC-16/XMODEM校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note poly=0x1021, init=0x0000, refin=false, refout=false, xorout=0x0000
    uint16_t crc16XMODEM(const uint8_t* data, size_t length);

    /// @brief 计算CRC-16/KERMIT校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note poly=0x1021, init=0x0000, refin=true, refout=true, xorout=0x0000
    uint16_t crc16KERMIT(const uint8_t* data, size_t length);

    /// @brief 计算CRC16校验(MODBUS)
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note poly=0x8005, init=0xFFFF, refin=true, refout=true, xorout=0x0000
    uint16_t crc16MODBUS(const uint8_t* data, size_t length);

    /// @brief 计算CRC16校验(ANSI)
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note CRC-16/ARC, poly=0x8005, init=0x0000, refin=true, refout=true, xorout=0x0000
    uint16_t crc16ANSI(const uint8_t* data, size_t length);

    /// @brief 计算CRC16校验(IBM)
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC16校验值
    /// @note CRC-16/ARC 的别名
    uint16_t crc16IBM(const uint8_t* data, size_t length);

    /// @brief 计算CRC-32/ISO-HDLC校验
    /// @param data 数据
    /// @param length 数据长度
    /// @return CRC32校验值
    /// @note CRC-32/IEEE, poly=0x04C11DB7, init=0xFFFFFFFF, refin=true, refout=true, xorout=0xFFFFFFFF
    uint32_t crc32IEEE(const uint8_t* data, size_t length);

} // CheckUtils

#endif // !__CHECK_UTILS_H__
