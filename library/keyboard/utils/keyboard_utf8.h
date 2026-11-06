/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:00:00
 * @LastEditTime: 2026-11-04 18:40:12
 * @FilePath: /kk_frame/library/keyboard/utils/keyboard_utf8.h
 * @Description: 键盘库自用的 UTF-8 按字符计数与截断（不引用应用层 utils）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_UTF8_H__
#define __KEYBOARD_UTF8_H__

#include <cstddef>
#include <string>

/// @brief 键盘输入长度限制所需的 UTF-8 字符操作
/// @note 非法序列一律按单字节处理（不丢字节、不抛错）
namespace KeyboardUtf8 {

    /// @brief 计算字符串的加权字符数
    /// @param str 字符串
    /// @param chineseWeight 非 ASCII 字符的权重（1 表示按字符计，2 表示占两格）
    /// @return 加权后的字符数
    size_t characterCount(const char* str, int chineseWeight = 1);

    /// @brief 截取不超过指定加权字符数的前缀
    /// @param str 字符串
    /// @param maxChars 最大加权字符数
    /// @param chineseWeight 非 ASCII 字符的权重
    /// @return 截取结果（不会截断半个多字节字符）
    std::string substringByChars(const char* str, size_t maxChars, int chineseWeight = 1);

} // KeyboardUtf8

#endif // !__KEYBOARD_UTF8_H__
