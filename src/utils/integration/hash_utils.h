/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:07
 * @LastEditTime: 2026-11-06 09:41:04
 * @FilePath: /kk_frame/src/utils/integration/hash_utils.h
 * @Description: 摘要算法（MD5 / SHA256）相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __HASH_UTILS_H__
#define __HASH_UTILS_H__

#include <cstddef>
#include <cstdint>
#include <string>

/// @brief 密码学摘要（MD5 / SHA256）
namespace HashUtils {

    // MD5校验长度
    constexpr uint8_t MD5_SIZE = 16;
    // MD5校验结果字符串长度
    constexpr uint8_t MD5_STR_LEN = (MD5_SIZE * 2);
    // SHA256校验长度
    constexpr uint8_t SHA256_SIZE = 32;
    // SHA256校验结果字符串长度
    constexpr uint8_t SHA256_STR_LEN = (SHA256_SIZE * 2);

    /// @brief 计算数据MD5校验
    /// @param data 数据地址
    /// @param len 数据长度
    /// @param md5Str MD5校验结果存储空间（长度 MD5_STR_LEN，不写结尾 '\0'）
    /// @return 成功返回 md5Str；失败（MD5 库未启用）返回 nullptr 且缓冲区被清零
    char* md5Check(const void* data, size_t len, char md5Str[MD5_STR_LEN]);

    /// @brief 计算文件MD5校验
    /// @param filePath 文件路径
    /// @param md5Str MD5校验结果存储空间（长度 MD5_STR_LEN，不写结尾 '\0'）
    /// @return 成功返回 md5Str；打开/读取失败或 MD5 库未启用返回 nullptr 且缓冲区被清零
    char* md5Check(const char* filePath, char md5Str[MD5_STR_LEN]);

    /// @brief 计算数据SHA256校验
    /// @param data 数据地址
    /// @param len 数据长度
    /// @param sha256Value SHA256校验结果存储空间
    /// @return 校验结果
    uint8_t* sha256Check(const void* data, size_t len, uint8_t sha256Value[SHA256_SIZE]);

    /// @brief 计算数据SHA256校验
    /// @param data 数据地址
    /// @param len 数据长度
    /// @return SHA256校验结果字符串（小写十六进制）
    std::string sha256CheckStr(const void* data, size_t len);

    /// @brief 计算数据SHA256校验
    /// @param src 数据
    /// @return SHA256校验结果字符串（小写十六进制）
    std::string sha256CheckStr(const std::string& src);

} // namespace HashUtils

#endif // !__HASH_UTILS_H__
