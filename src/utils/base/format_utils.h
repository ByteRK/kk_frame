/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-05 14:44:16
 * @FilePath: /kk_frame/src/utils/base/format_utils.h
 * @Description: 展示用格式化（十六进制、容量单位）相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FORMAT_UTILS_H__
#define __FORMAT_UTILS_H__

#include <cstddef>
#include <string>

/// @brief 面向"展示"的格式化函数
namespace FormatUtils {

    /// @brief 将字节数组转换为16进制字符串
    /// @param buf 字节数组
    /// @param len 字节数组长度
    /// @param width 每行显示的字节数(小于1不换行)
    /// @return 16进制字符串（每字节后带一个空格）
    std::string hexStr(const unsigned char* buf, int len, int width = 30);

    /// @brief 打印字节数组，以16进制形式输出
    /// @param label 标签
    /// @param buf 字节数组
    /// @param len 字节数组长度
    /// @param width 每行显示的字节数(小于1不换行)
    void hexdump(const char* label, const unsigned char* buf, int len, int width = 30);

    /// @brief 字节数可视化，如1024 -> 1.00 KB
    /// @param byteSize 字节数
    /// @param precision 进位后保留的小数位数（字节单位不保留小数）
    /// @param includeTB 是否允许进位到 TB
    /// @return 可视化后的字符串
    std::string sizeVisualization(size_t byteSize, int precision = 2, bool includeTB = false);

} // FormatUtils

#endif // !__FORMAT_UTILS_H__
