/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 16:07:53
 * @LastEditTime: 2026-02-24 02:16:04
 * @FilePath: /kk_frame/src/utils/string_utils.cc
 * @Description: 字符串相关的一些操作函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "string_utils.h"
#include <cdlog.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdint>


/// @brief UTF-8字符结构体，用于统一处理
struct Utf8Char {
    const char* start;  // 字符起始位置
    int length;         // 字符字节长度
    bool valid;         // 是否为有效的UTF-8字符
};

/// @brief 获取下一个UTF-8字符的信息
/// @param pos 当前位置指针
/// @return Utf8Char结构体，包含字符信息和有效性
inline Utf8Char nextUtf8Char(const unsigned char* pos) {
    Utf8Char ch = { reinterpret_cast<const char*>(pos), 1, true };
    if (*pos == 0) {
        ch.valid = false;
        return ch;
    }
    // ASCII字符
    if (*pos < 0x80) {
        return ch;
    }
    // 多字节字符
    if ((*pos & 0xE0) == 0xC0) {
        ch.length = 2;
    } else if ((*pos & 0xF0) == 0xE0) {
        ch.length = 3;
    } else if ((*pos & 0xF8) == 0xF0) {
        ch.length = 4;
    } else {
        // 无效的UTF-8起始字节
        ch.valid = false;
        return ch;
    }
    // 验证后续字节
    for (int i = 1; i < ch.length; i++) {
        if ((pos[i] & 0xC0) != 0x80) {
            ch.valid = false;
            ch.length = 1;  // 当作单字节处理
            break;
        }
    }
    // 验证4字节字符的范围
    if (ch.length == 4 && ch.valid) {
        uint32_t code_point = ((pos[0] & 0x07) << 18) |
            ((pos[1] & 0x3F) << 12) |
            ((pos[2] & 0x3F) << 6) |
            (pos[3] & 0x3F);
        if (code_point > 0x10FFFF) {
            ch.valid = false;
            ch.length = 1;
        }
    }
    return ch;
}


bool StringUtils::isNumric(const char* str) {
    // 空字符串或NULL返回false
    if (str == NULL || *str == '\0') return false;
    bool hasDecimal = false; // 标记是否已遇到小数点
    bool hasDigits = false;  // 标记是否已遇到数字
    if (*str == '-') str++; // 跳过可能的负号
    while (*str) { // 遍历字符串
        if (isdigit(*str)) {
            hasDigits = true; // 找到数字
        } else if (*str == '.') {
            if (hasDecimal) {
                return false; // 如果已经有小数点，则返回false
            }
            hasDecimal = true; // 找到小数点
        } else {
            return false; // 如果是其他字符，返回false
        }
        str++;
    }
    return hasDigits; // 至少要有一个数字
}

void StringUtils::upper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
}

std::string StringUtils::upperNew(const std::string& str) {
    std::string upperStr = str;
    upper(upperStr);
    return upperStr;
}

void StringUtils::lower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

std::string StringUtils::lowerNew(const std::string& str) {
    std::string lowerStr = str;
    lower(lowerStr);
    return lowerStr;
}

std::string StringUtils::format(const char* format, ...) {
    static char fmt_str[1025];
    va_list     args;
    va_start(args, format);
    vsnprintf(fmt_str, sizeof(fmt_str), format, args);
    va_end(args);
    return std::string(fmt_str);
}

std::string StringUtils::fill(const int& num, int len, char pre) {
    std::string retString, numString = std::to_string(num);
    if (numString.length() < len) retString.append(len - numString.length(), pre);
    retString += numString;
    return retString;
}

std::string StringUtils::fill(const std::string& str, int len, char end) {
    std::string retString(str);
    if (retString.length() < len) retString.append(len - retString.size(), end);
    return retString;
}

std::vector<std::string> StringUtils::split(const std::string& str, const char delimiter) {
    int    offset = 0;
    std::vector<std::string> output;
    size_t pos = str.find(delimiter, offset);
    while (pos != std::string::npos) {
        if (offset < pos) output.push_back(str.substr(offset, pos - offset));
        offset = pos + 1;
        pos = str.find(delimiter, offset);
    }
    if (offset < str.size()) { output.push_back(str.substr(offset)); }
    return output;
}

std::string StringUtils::replace(const std::string& src, const std::string& oldStr, const std::string& newStr) {
    if (oldStr.empty()) return src;
    std::ostringstream result;
    std::string::size_type pos = 0;
    std::string::size_type length = oldStr.length();
    while ((pos = src.find(oldStr, pos)) != std::string::npos) {
        result << src.substr(0, pos);
        result << newStr;
        pos += length;
    }
    result << src.substr(pos);
    return result.str();
}

std::string StringUtils::remove(std::string str, const char remove) {
    str.erase(std::remove_if(str.begin(), str.end(), [remove](char ch) {
        return ch == remove;
    }), str.end());
    return str;
}

std::string StringUtils::trimRight(std::string str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !(std::isspace(ch) || ch == '\n' || ch == '\r' || ch == '\t');
    });
    str.erase(it.base(), str.end());
    return str;
}

const char* StringUtils::strcasestr(const char* haystack, const char* needle) {
    if (!*needle) return haystack; // 如果 str2 为空，返回 str1
    while (*haystack) {
        const char* s1 = haystack;
        const char* s2 = needle;
        // 比较字符，直到找到不匹配的字符或到达字符串末尾
        while (*s1 && *s2 && std::toupper(static_cast<unsigned char>(*s1)) == std::toupper(static_cast<unsigned char>(*s2))) {
            s1++;
            s2++;
        }
        // 如果 s2 到达末尾，说明找到了匹配
        if (!*s2) return haystack;
        haystack++; // 移动到 str1 的下一个字符
    }
    return nullptr; // 没有找到匹配
}

void StringUtils::hexdump(const char* label, const unsigned char* buf, int len, int width) {
    if (label == NULL || *label == '\0')
        LOG(DEBUG) << "hex dump:";
    else
        LOG(DEBUG) << label << " hex dump:";
    LOG(DEBUG) << hexStr(buf, len, width);
}

std::string StringUtils::hexStr(const unsigned char* buf, int len, int width) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0'); // 设置格式
    // 计算最终字符串的长度并预留内存
    int totalLength = len * 3 + (len / width) + 1; // 每个字节需要2个字符加空格，换行符
    std::string result;
    result.reserve(totalLength);
    for (int i = 0; i < len; i++) {
        oss << std::setw(2) << static_cast<int>(buf[i]) << " ";
        // 如果width大于0且是width的倍数，添加换行符
        if (width > 0 && (i + 1) % width == 0) oss << '\n';
    }
    result = oss.str();
    return result;
}

size_t StringUtils::characterCount(const char* str, int chineseWeight) {
    size_t count = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
    while (*p) {
        Utf8Char ch = nextUtf8Char(p);
        if (ch.valid) {
            // 如果字符是多字节且设置了中文权重>1，则按权重计算
            count += (ch.length > 1 && chineseWeight > 1) ? chineseWeight : 1;
        } else {
            // 无效字符按1计数
            count++;
        }
        p += ch.length;
    }
    return count;
}

std::string StringUtils::substringByChars(const char* str, size_t maxChars, int chineseWeight) {
    if (!str || maxChars == 0) return "";
    const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
    const unsigned char* start = p;
    size_t charCount = 0;
    size_t byteCount = 0;
    while (*p && charCount < maxChars) {
        Utf8Char ch = nextUtf8Char(p);
        // 检查添加这个字符后是否会超过限制
        size_t charWeight = (ch.length > 1 && chineseWeight > 1) ? chineseWeight : 1;
        if (charCount + charWeight > maxChars) {
            break;
        }
        charCount += charWeight;
        byteCount += ch.length;
        p += ch.length;
    }
    return std::string(reinterpret_cast<const char*>(start), byteCount);
}

std::string StringUtils::removeLastCharacter(const char* str) {
    if (!str || *str == '\0') return "";

    const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
    const unsigned char* lastCharStart = nullptr;
    const unsigned char* current = p;

    // 遍历找到最后一个字符的起始位置
    while (*current) {
        Utf8Char ch = nextUtf8Char(current);
        if (!ch.valid) {
            // 无效字符，跳过这个字节
            current++;
            continue;
        }

        lastCharStart = current;
        current += ch.length;
    }

    // 如果没有找到有效字符或只有一个字符
    if (lastCharStart == nullptr || lastCharStart == p) {
        return "";
    }

    // 返回删除最后一个字符后的字符串
    return std::string(str, lastCharStart - p);
}

void StringUtils::popLastCharacter(std::string& str) {
    if (str.empty()) return;

    const unsigned char* data = reinterpret_cast<const unsigned char*>(str.data());
    size_t len = str.size();
    const unsigned char* lastCharStart = nullptr;
    size_t pos = 0;

    // 遍历找到最后一个有效字符的起始位置
    while (pos < len) {
        const unsigned char* charStart = data + pos;

        // 获取当前字符信息
        Utf8Char ch = nextUtf8Char(charStart);

        if (!ch.valid) {
            // 无效字符，跳过这个字节
            pos++;
            continue;
        }

        // 检查这个字符是否完整在字符串内
        if (pos + ch.length <= len) {
            lastCharStart = charStart;
            pos += ch.length;
        } else {
            // 字符不完整，跳出循环
            break;
        }
    }

    // 如果找到了有效字符，删除它
    if (lastCharStart != nullptr) {
        if (lastCharStart == data) {
            // 整个字符串只有一个字符
            str.clear();
        } else {
            // 删除最后一个字符
            size_t newSize = lastCharStart - data;
            str.resize(newSize);
        }
    }
}

std::string StringUtils::truncateWithLimit(const char* str, size_t maxChars, int chineseWeight, const std::string& suffix) {
    if (!str || maxChars == 0) {
        return suffix.empty() ? "" : suffix;
    }
    // 先计算是否超过限制
    size_t totalChars = characterCount(str, chineseWeight);
    if (totalChars <= maxChars) {
        return std::string(str);
    }
    // 计算保留的字符数（考虑后缀）
    size_t suffixCharCount = characterCount(suffix.c_str(), chineseWeight);
    size_t keepChars = (suffixCharCount < maxChars) ? maxChars - suffixCharCount : 1;
    // 获取截取部分
    std::string result = substringByChars(str, keepChars, chineseWeight);
    // 如果截取后需要删除不完整的字符
    if (!result.empty() && result != str) {
        // 移除可能被截断的最后一个字符
        result = removeLastCharacter(result.c_str());
    }
    // 添加后缀
    if (!suffix.empty() && keepChars > 0) {
        result += suffix;
    }
    return result;
}
