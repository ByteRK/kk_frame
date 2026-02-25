/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:38:20
 * @LastEditTime: 2026-02-26 02:25:16
 * @FilePath: /kk_frame/src/utils/encoding_utils.h
 * @Description: 编码相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ENCODING_UTILS_H__
#define __ENCODING_UTILS_H__

#include <string>
#include <stdint.h>

namespace EncodingUtils {

    /// @brief 检查当前系统是否为小端字节序
    /// @return 
    inline bool isLittleEndian() {
        const uint16_t test = 0x0102;
        return *(reinterpret_cast<const uint8_t*>(&test)) == 0x02;
    }

    /* 字节序反转 */
    constexpr uint16_t bswap16(uint16_t v) {
        return (v >> 8) | (v << 8);
    }
    constexpr uint32_t bswap32(uint32_t v) {
        return ((v & 0x000000FFu) << 24) |
            ((v & 0x0000FF00u) << 8) |
            ((v & 0x00FF0000u) >> 8) |
            ((v & 0xFF000000u) >> 24);
    }
    constexpr uint64_t bswap64(uint64_t v) {
        return ((v & 0x00000000000000FFull) << 56) |
            ((v & 0x000000000000FF00ull) << 40) |
            ((v & 0x0000000000FF0000ull) << 24) |
            ((v & 0x00000000FF000000ull) << 8) |
            ((v & 0x000000FF00000000ull) >> 8) |
            ((v & 0x0000FF0000000000ull) >> 24) |
            ((v & 0x00FF000000000000ull) >> 40) |
            ((v & 0xFF00000000000000ull) >> 56);
    }

    /* 机器序与大端序智能互换 */
    inline uint16_t hostToBig16(uint16_t v) { return isLittleEndian() ? bswap16(v) : v; }
    inline uint32_t hostToBig32(uint32_t v) { return isLittleEndian() ? bswap32(v) : v; }
    inline uint64_t hostToBig64(uint64_t v) { return isLittleEndian() ? bswap64(v) : v; }
    inline uint16_t bigToHost16(uint16_t v) { return hostToBig16(v); }
    inline uint32_t bigToHost32(uint32_t v) { return hostToBig32(v); }
    inline uint64_t bigToHost64(uint64_t v) { return hostToBig64(v); }

    /* 机器序与小端序智能互换 */
    inline uint16_t hostToLittle16(uint16_t v) { return isLittleEndian() ? v : bswap16(v); }
    inline uint32_t hostToLittle32(uint32_t v) { return isLittleEndian() ? v : bswap32(v); }
    inline uint64_t hostToLittle64(uint64_t v) { return isLittleEndian() ? v : bswap64(v); }
    inline uint16_t littleToHost16(uint16_t v) { return hostToLittle16(v); }
    inline uint32_t littleToHost32(uint32_t v) { return hostToLittle32(v); }
    inline uint64_t littleToHost64(uint64_t v) { return hostToLittle64(v); }

    
    /// @brief 将字符串从一种编码转换为另一种编码
    /// @param input 要转换的字符串
    /// @param fromEncoding 原始编码
    /// @param toEncoding 目标编码
    /// @return 转换后的字符串
    std::string convert(const std::string& input, const char* fromEncoding, const char* toEncoding);

    /// @brief 将 "\xHH" 格式的字符串转换为实际字节流
    /// @param input 
    /// @return 
    /// @note 当看到文本显示为类似 "\xe4\xbd\xa0\xe5\xa5\xbd" 时，可以调用此函数将其转换为实际的字节流
    std::string hexEscapes(const std::string& input);

} // EncodingUtils

#endif // !__ENCODING_UTILS_H__