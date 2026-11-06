/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:38:20
 * @LastEditTime: 2026-11-06 09:35:08
 * @FilePath: /kk_frame/src/utils/base/bytes_utils.h
 * @Description: 字节级工具（线路字节序读写、十六进制编解码）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BYTES_UTILS_H__
#define __BYTES_UTILS_H__

#include <string>
#include <stdint.h>
#include <string.h>

/// @brief 字节级工具：多字节整数的线路字节序读写 + 十六进制编解码
/// @note 本模块按主题分区：
///       1) 字节序读写   2) 十六进制编解码
namespace BytesUtils {

    /// @brief 多字节数据的线路字节序
    typedef enum {
        BigEndian,    // 大端字节序
        LittleEndian, // 小端字节序
    } ByteOrder;

    /// @brief 十六进制字符转数值
    /// @param ch 待转换字符
    /// @return 0~15；非十六进制字符返回 -1
    inline int hexValue(char ch) {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    }

    /// @brief 检查当前系统是否为小端字节序
    /// @return 
    inline bool isLittleEndian() {
        const uint16_t test = 0x0102;
        return *(reinterpret_cast<const uint8_t*>(&test)) == 0x02;
    }

    /* 字节序反转 */
    inline uint16_t bswap16(uint16_t v) {
        return (v >> 8) | (v << 8);
    }
    inline uint32_t bswap32(uint32_t v) {
        return ((v & 0x000000FFu) << 24) |
            ((v & 0x0000FF00u) << 8) |
            ((v & 0x00FF0000u) >> 8) |
            ((v & 0xFF000000u) >> 24);
    }
    inline uint64_t bswap64(uint64_t v) {
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

    /* 按大端字节序从字节块读取整数，并转换为机器序；允许非对齐地址 */
    inline uint16_t readBig16(const void* data) {
        uint16_t value;
        memcpy(&value, data, sizeof(value));
        return bigToHost16(value);
    }
    inline uint32_t readBig32(const void* data) {
        uint32_t value;
        memcpy(&value, data, sizeof(value));
        return bigToHost32(value);
    }
    inline uint64_t readBig64(const void* data) {
        uint64_t value;
        memcpy(&value, data, sizeof(value));
        return bigToHost64(value);
    }

    /* 按小端字节序从字节块读取整数，并转换为机器序；允许非对齐地址 */
    inline uint16_t readLittle16(const void* data) {
        uint16_t value;
        memcpy(&value, data, sizeof(value));
        return littleToHost16(value);
    }
    inline uint32_t readLittle32(const void* data) {
        uint32_t value;
        memcpy(&value, data, sizeof(value));
        return littleToHost32(value);
    }
    inline uint64_t readLittle64(const void* data) {
        uint64_t value;
        memcpy(&value, data, sizeof(value));
        return littleToHost64(value);
    }

    /* 按指定字节序从字节块读取整数，并转换为机器序；允许非对齐地址 */
    inline uint16_t read16(const void* data, ByteOrder order) {
        return (order == ByteOrder::BigEndian) ? readBig16(data) : readLittle16(data);
    }
    inline uint32_t read32(const void* data, ByteOrder order) {
        return (order == ByteOrder::BigEndian) ? readBig32(data) : readLittle32(data);
    }
    inline uint64_t read64(const void* data, ByteOrder order) {
        return (order == ByteOrder::BigEndian) ? readBig64(data) : readLittle64(data);
    }

    /* 将机器序整数按大端字节序写入字节块；允许非对齐地址 */
    inline void writeBig16(uint16_t value, void* data) {
        value = hostToBig16(value);
        memcpy(data, &value, sizeof(value));
    }
    inline void writeBig32(uint32_t value, void* data) {
        value = hostToBig32(value);
        memcpy(data, &value, sizeof(value));
    }
    inline void writeBig64(uint64_t value, void* data) {
        value = hostToBig64(value);
        memcpy(data, &value, sizeof(value));
    }

    /* 将机器序整数按小端字节序写入字节块；允许非对齐地址 */
    inline void writeLittle16(uint16_t value, void* data) {
        value = hostToLittle16(value);
        memcpy(data, &value, sizeof(value));
    }
    inline void writeLittle32(uint32_t value, void* data) {
        value = hostToLittle32(value);
        memcpy(data, &value, sizeof(value));
    }
    inline void writeLittle64(uint64_t value, void* data) {
        value = hostToLittle64(value);
        memcpy(data, &value, sizeof(value));
    }

    /* 将机器序整数按指定字节序写入字节块；允许非对齐地址 */
    inline void write16(uint16_t value, void* data, ByteOrder order) {
        if (order == ByteOrder::BigEndian) writeBig16(value, data);
        else writeLittle16(value, data);
    }
    inline void write32(uint32_t value, void* data, ByteOrder order) {
        if (order == ByteOrder::BigEndian) writeBig32(value, data);
        else writeLittle32(value, data);
    }
    inline void write64(uint64_t value, void* data, ByteOrder order) {
        if (order == ByteOrder::BigEndian) writeBig64(value, data);
        else writeLittle64(value, data);
    }

    /// @brief 将 "\xHH" 格式的字符串转换为实际字节流
    /// @param input 待转换字符串
    /// @return 转换后的字节流
    /// @note 当看到文本显示为类似 "\xe4\xbd\xa0\xe5\xa5\xbd" 时，可以调用此函数将其转换为实际的字节流
    std::string hexEscapes(const std::string& input);

    /// @brief 将字节数据编码为连续的十六进制字符串（小写，无分隔符）
    /// @param data 数据指针
    /// @param length 数据长度
    /// @return 十六进制字符串
    std::string hexEncode(const void* data, size_t length);

    /// @brief 将字符串内容按字节编码为连续的十六进制字符串
    /// @param data 字符串数据
    /// @return 十六进制字符串
    std::string hexEncode(const std::string& data);

    /// @brief 将十六进制字符串解码为字节流
    /// @param hex 十六进制字符串
    /// @param out 解码结果（每 2 个十六进制字符为 1 字节）
    /// @param allowSeparators 是否允许空格、冒号、短横线分隔（如 "AA BB"、"AA:BB"）
    /// @return true 解码成功；非法字符或奇数个有效字符时返回 false（out 保持不变）
    /// @note 与 hexEncode 对称：hexDecode(hexEncode(x)) == x
    bool hexDecode(const std::string& hex, std::string& out, bool allowSeparators = false);

} // BytesUtils

#endif // !__BYTES_UTILS_H__
