/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-05 19:45:00
 * @FilePath: /kk_frame/src/utils/platform/sysnode_utils.h
 * @Description: 系统虚拟文件节点（/sys、/proc）读写
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __SYSNODE_UTILS_H__
#define __SYSNODE_UTILS_H__

#include <cstdint>
#include <string>

/// @brief 系统虚拟文件节点读写（覆盖 /sys 与 /proc）
/// @note 所有读取接口都会对内容做 trim（去除首尾空白与换行）后再解析，
///       因此不适合用于读取二进制数据；读二进制请使用 FileUtils::read。
/// @note 分工：
///       - 单值节点（亮度 / 温度 / 布尔开关）→ readInt / readBool / write
///       - 多行文本（/proc/meminfo、/proc/cpuinfo）→ readTextFile
namespace SysNodeUtils {

    /// @brief 写入值到系统路径
    /// @param path 路径
    /// @param value 值
    /// @return true 写入成功
    /// @note 可替代echo，节约消耗
    bool write(const std::string& path, const std::string& value);

    /// @brief 读取系统路径中的整数值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的整数，失败返回 def
    /// @note 可替代cat，节约消耗
    int readInt(const std::string& path, int def = 0);

    /// @brief 读取系统路径中的 64 位整数值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的整数，失败返回 def
    /// @note 可替代cat，节约消耗
    int64_t readInt64(const std::string& path, int64_t def = 0);

    /// @brief 读取系统路径中的浮点值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的浮点数，失败返回 def
    /// @note 可替代cat，节约消耗
    double readDouble(const std::string& path, double def = 0.0);

    /// @brief 读取系统路径中的字符串
    /// @param path 路径
    /// @param def 读取失败或内容为空时的默认值
    /// @return 读取到的字符串（已去除首尾空白与换行），失败返回 def
    /// @note 可替代cat，节约消耗
    std::string readString(const std::string& path, const std::string& def = "");

    /// @brief 读取系统路径中的布尔值
    /// @param path 路径
    /// @param def 读取失败或无法识别时的默认值
    /// @return 1/Y/YES/ON/TRUE/ENABLE 等视为 true，0/N/NO/OFF/FALSE/DISABLE 等视为 false，
    ///         无法识别返回 def
    /// @note 可替代cat，节约消耗
    bool readBool(const std::string& path, bool def = false);

    /// @brief 读取虚拟文件完整内容（适用于 /proc/meminfo、/proc/cpuinfo 等多行文本）
    /// @param path 路径
    /// @param content 读取结果，已去除首尾空白与换行
    /// @return true 成功
    bool readTextFile(const std::string& path, std::string& content);

} // SysNodeUtils

#endif // !__SYSNODE_UTILS_H__
