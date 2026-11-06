/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:00:00
 * @LastEditTime: 2026-11-04 18:39:53
 * @FilePath: /kk_frame/library/keyboard/utils/keyboard_utf8.cc
 * @Description: 键盘库自用的 UTF-8 按字符计数与截断（不引用应用层 utils）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_utf8.h"

#include <cstdint>
#include <cstring>

namespace {

    // 注意：本文件与 src/utils/base/string_utils.cc 中的 UTF-8 部分是同一套逻辑的
    //      两次实现（库不能反向依赖应用层 utils）。
    //      修改字符解析/权重规则时（nextUtf8Char / charWeight）两边必须同步。

    /// @brief UTF-8 字符基本信息
    struct Utf8Char {
        int  length; // 占用字节数
        bool valid;  // 是否为合法序列
    };

    /// @brief 解析 pos 处的 UTF-8 字符
    /// @param pos 当前位置
    /// @param remaining 剩余字节数
    /// @return 字符信息；非法序列按单字节返回
    Utf8Char nextUtf8Char(const unsigned char* pos, size_t remaining) {
        Utf8Char ch = { 1, true };
        if (pos == nullptr || remaining == 0 || *pos == 0) {
            ch.valid = false;
            return ch;
        }
        if (*pos < 0x80) {
            return ch;
        }

        if ((*pos & 0xE0) == 0xC0) {
            ch.length = 2;
        } else if ((*pos & 0xF0) == 0xE0) {
            ch.length = 3;
        } else if ((*pos & 0xF8) == 0xF0) {
            ch.length = 4;
        } else {
            ch.valid = false;
            return ch;
        }

        if (static_cast<size_t>(ch.length) > remaining) {
            ch.valid = false;
            ch.length = 1;
            return ch;
        }

        for (int i = 1; i < ch.length; i++) {
            if ((pos[i] & 0xC0) != 0x80) {
                ch.valid = false;
                ch.length = 1;
                break;
            }
        }

        // 验证 4 字节字符的码点上限，避免把超出 Unicode 范围的序列当成合法字符
        if (ch.length == 4 && ch.valid) {
            const uint32_t codePoint = ((pos[0] & 0x07) << 18) |
                ((pos[1] & 0x3F) << 12) |
                ((pos[2] & 0x3F) << 6) |
                (pos[3] & 0x3F);
            if (codePoint > 0x10FFFF) {
                ch.valid = false;
                ch.length = 1;
            }
        }

        return ch;
    }

    /// @brief 计算单个字符的权重
    size_t charWeight(const Utf8Char& ch, int chineseWeight) {
        return (ch.valid && ch.length > 1 && chineseWeight > 1)
            ? static_cast<size_t>(chineseWeight) : 1;
    }

} // namespace

size_t KeyboardUtf8::characterCount(const char* str, int chineseWeight) {
    if (str == nullptr) return 0;

    const unsigned char* data = reinterpret_cast<const unsigned char*>(str);
    const size_t length = std::strlen(str);
    size_t pos = 0;
    size_t count = 0;

    while (pos < length) {
        const Utf8Char ch = nextUtf8Char(data + pos, length - pos);
        count += charWeight(ch, chineseWeight);
        pos += static_cast<size_t>(ch.length);
    }
    return count;
}

std::string KeyboardUtf8::substringByChars(const char* str, size_t maxChars, int chineseWeight) {
    if (str == nullptr || maxChars == 0) return std::string();

    const unsigned char* data = reinterpret_cast<const unsigned char*>(str);
    const size_t length = std::strlen(str);
    size_t pos = 0;
    size_t count = 0;

    while (pos < length) {
        const Utf8Char ch = nextUtf8Char(data + pos, length - pos);
        const size_t weight = charWeight(ch, chineseWeight);
        if (count + weight > maxChars) break;
        count += weight;
        pos += static_cast<size_t>(ch.length);
    }
    return std::string(reinterpret_cast<const char*>(data), pos);
}
