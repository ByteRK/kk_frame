/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 16:07:53
 * @LastEditTime: 2026-11-05 19:10:00
 * @FilePath: /kk_frame/src/utils/base/string_utils.cc
 * @Description: 字符串工具（字节操作 / UTF-8 字符操作 / 编码转换）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "string_utils.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

#include <iconv.h>

namespace {

    /// @brief 布尔字面量表（小写），EnvUtils / SysNodeUtils 共用
    const char* const kTrueValues[] = {
        "1", "y", "yes", "true", "on", "enable", "enabled"
    };
    const char* const kFalseValues[] = {
        "0", "n", "no", "false", "off", "disable", "disabled"
    };

} // namespace

bool StringUtils::isNumeric(const char* str) {
    // 空字符串或NULL返回false
    if (str == NULL || *str == '\0') return false;
    bool hasDecimal = false; // 标记是否已遇到小数点
    bool hasDigits = false;  // 标记是否已遇到数字
    if (*str == '-') str++; // 跳过可能的负号
    while (*str) { // 遍历字符串
        // isdigit 只接受 unsigned char / EOF，直接传 char 在多字节文本下是 UB
        if (isdigit(static_cast<unsigned char>(*str))) {
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

bool StringUtils::toBool(const std::string& text, bool& out) {
    const std::string key = lowerNew(trim(text));

    for (size_t i = 0; i < sizeof(kTrueValues) / sizeof(kTrueValues[0]); ++i) {
        if (key == kTrueValues[i]) {
            out = true;
            return true;
        }
    }
    for (size_t i = 0; i < sizeof(kFalseValues) / sizeof(kFalseValues[0]); ++i) {
        if (key == kFalseValues[i]) {
            out = false;
            return true;
        }
    }
    return false;
}

bool StringUtils::toInt(const std::string& text, int& out) {
    int64_t value = 0;
    if (!toInt64(text, value)) return false;
    if (value < INT_MIN || value > INT_MAX) return false;
    out = static_cast<int>(value);
    return true;
}

bool StringUtils::toInt64(const std::string& text, int64_t& out) {
    const std::string trimmed = trim(text);
    if (trimmed.empty()) return false;

    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(trimmed.c_str(), &end, 10);
    if (errno != 0 || end == trimmed.c_str() || *end != '\0') return false;

    out = static_cast<int64_t>(value);
    return true;
}

bool StringUtils::toDouble(const std::string& text, double& out) {
    const std::string trimmed = trim(text);
    if (trimmed.empty()) return false;

    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(trimmed.c_str(), &end);
    if (errno != 0 || end == trimmed.c_str() || *end != '\0') return false;

    out = value;
    return true;
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
    if (format == nullptr) return "";

    va_list args;
    va_start(args, format);
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int size = std::vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);
    if (size < 0) {
        va_end(args);
        return "";
    }

    std::vector<char> buffer(static_cast<size_t>(size) + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
    return std::string(buffer.data(), static_cast<size_t>(size));
}

std::string StringUtils::fill(const int& num, int len, char pre) {
    std::string retString, numString = std::to_string(num);
    const size_t targetLen = (len > 0) ? static_cast<size_t>(len) : 0;
    if (numString.length() < targetLen) {
        retString.append(targetLen - numString.length(), pre);
    }
    retString += numString;
    return retString;
}

std::string StringUtils::fill(const std::string& str, int len, char end) {
    std::string retString(str);
    const size_t targetLen = (len > 0) ? static_cast<size_t>(len) : 0;
    if (retString.length() < targetLen) {
        retString.append(targetLen - retString.size(), end);
    }
    return retString;
}

std::vector<std::string> StringUtils::split(const std::string& str, const char delimiter) {
    std::vector<std::string> output;
    size_t offset = 0;
    size_t pos = str.find(delimiter, offset);
    while (pos != std::string::npos) {
        if (offset < pos) output.push_back(str.substr(offset, pos - offset));
        offset = pos + 1;
        pos = str.find(delimiter, offset);
    }
    if (offset < str.size()) { output.push_back(str.substr(offset)); }
    return output;
}

std::vector<std::string> StringUtils::splitAny(const std::string& str, const char* delimiters) {
    std::vector<std::string> output;
    if (delimiters == nullptr || *delimiters == '\0') {
        if (!str.empty()) output.push_back(str);
        return output;
    }

    size_t offset = 0;
    while (offset <= str.size()) {
        size_t end = str.find_first_of(delimiters, offset);
        if (end == std::string::npos) end = str.size();

        if (offset < end) output.push_back(str.substr(offset, end - offset));
        if (end == str.size()) break;

        offset = end + 1;
    }
    return output;
}

std::string StringUtils::join(const std::vector<std::string>& parts, const char* separator) {
    const char* sep = (separator == nullptr) ? "" : separator;

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += sep;
        result += parts[i];
    }
    return result;
}

std::string StringUtils::replace(const std::string& src, const std::string& oldStr, const std::string& newStr) {
    if (oldStr.empty()) return src;
    std::ostringstream result;
    std::string::size_type searchPos = 0;
    std::string::size_type lastPos = 0;
    const std::string::size_type length = oldStr.length();
    while ((searchPos = src.find(oldStr, searchPos)) != std::string::npos) {
        result << src.substr(lastPos, searchPos - lastPos);
        result << newStr;
        searchPos += length;
        lastPos = searchPos;
    }
    result << src.substr(lastPos);
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

std::string StringUtils::trimLeft(std::string str) {
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !(std::isspace(ch) || ch == '\n' || ch == '\r' || ch == '\t');
    });
    str.erase(str.begin(), it);
    return str;
}

std::string StringUtils::trim(std::string str) {
    return trimLeft(trimRight(str));
}

void StringUtils::trimInPlace(std::string& str) {
    // trimLeft/trimRight 按值传入并返回处理结果，此处必须回写
    str = trimRight(trimLeft(std::move(str)));
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size()
        && str.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool StringUtils::equalsIgnoreCase(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size()) return false;

    for (size_t i = 0; i < lhs.size(); ++i) {
        const unsigned char a = static_cast<unsigned char>(lhs[i]);
        const unsigned char b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return true;
}

bool StringUtils::endsWithIgnoreCase(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;

    const size_t offset = str.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        const unsigned char lhs = static_cast<unsigned char>(str[offset + i]);
        const unsigned char rhs = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(lhs) != std::tolower(rhs)) return false;
    }
    return true;
}

const char* StringUtils::strcasestr(const char* haystack, const char* needle) {
    if (haystack == nullptr || needle == nullptr) return nullptr;
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

/* ============================================================================
 * UTF-8 字符级操作
** ========================================================================== */

namespace {

    /// @brief UTF-8 字符结构体，用于统一处理
    struct Utf8Char {
        const char* start;  // 字符起始位置
        int         length; // 字符字节长度
        bool        valid;  // 是否为有效的UTF-8字符
    };

    /// @brief 获取下一个UTF-8字符的信息
    /// @param pos 当前位置指针
    /// @param remaining 剩余可读取字节数
    /// @return Utf8Char 结构体，包含字符信息和有效性；非法起始字节按单字节返回
    Utf8Char nextUtf8Char(const unsigned char* pos, size_t remaining) {
        Utf8Char ch = { reinterpret_cast<const char*>(pos), 1, true };
        if (pos == nullptr || remaining == 0 || *pos == 0) {
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
        if (static_cast<size_t>(ch.length) > remaining) {
            ch.valid = false;
            ch.length = 1;
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

    /// @brief 计算单个 UTF-8 字符的权重（非法字符固定按 1 计）
    /// @param ch 字符信息
    /// @param chineseWeight 中文字符权重
    /// @return 该字符占用的字符数
    size_t charWeight(const Utf8Char& ch, int chineseWeight) {
        return (ch.valid && ch.length > 1 && chineseWeight > 1)
            ? static_cast<size_t>(chineseWeight) : 1;
    }

    /// @brief 查找最后一个有效字符的起始偏移
    /// @param str 字符串
    /// @param length 可读取字节数
    /// @param offset 成功时写入最后一个有效字符的起始偏移
    /// @return true 存在有效字符
    /// @note 完全按 length 遍历，不把内嵌 '\0' 当作串尾
    bool lastCharOffset(const char* str, size_t length, size_t& offset) {
        const unsigned char* data = reinterpret_cast<const unsigned char*>(str);
        size_t pos = 0;
        bool found = false;

        while (pos < length) {
            const Utf8Char ch = nextUtf8Char(data + pos, length - pos);
            if (ch.valid) {
                offset = pos;
                found = true;
            }
            pos += static_cast<size_t>(ch.length);
        }
        return found;
    }

} // namespace

size_t StringUtils::characterCount(const char* str, int chineseWeight) {
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

std::string StringUtils::substringByChars(const char* str, size_t maxChars, int chineseWeight) {
    if (str == nullptr || maxChars == 0) return "";

    const unsigned char* data = reinterpret_cast<const unsigned char*>(str);
    const size_t length = std::strlen(str);
    size_t pos = 0;
    size_t charCount = 0;

    while (pos < length) {
        const Utf8Char ch = nextUtf8Char(data + pos, length - pos);
        const size_t weight = charWeight(ch, chineseWeight);
        // 检查添加这个字符后是否会超过限制
        if (charCount + weight > maxChars) break;
        charCount += weight;
        pos += static_cast<size_t>(ch.length);
    }
    return std::string(reinterpret_cast<const char*>(data), pos);
}

std::string StringUtils::removeLastCharacter(const char* str) {
    if (str == nullptr || *str == '\0') return "";

    size_t offset = 0;
    if (!lastCharOffset(str, std::strlen(str), offset) || offset == 0) {
        return "";
    }
    return std::string(str, offset);
}

void StringUtils::popLastCharacter(std::string& str) {
    if (str.empty()) return;

    size_t offset = 0;
    // 与 removeLastCharacter 的返回值语义保持一致：
    // 没有有效字符时同样删成空串，而不是把非法字节留在原地
    if (!lastCharOffset(str.data(), str.size(), offset)) {
        str.clear();
        return;
    }
    str.resize(offset);
}

std::string StringUtils::truncateWithLimit(const char* str, size_t maxChars,
    int chineseWeight, const std::string& suffix) {
    if (str == nullptr || maxChars == 0) {
        return "";
    }
    // 先计算是否超过限制
    const size_t totalChars = characterCount(str, chineseWeight);
    if (totalChars <= maxChars) {
        return std::string(str);
    }
    // 计算保留的字符数（考虑后缀）
    const size_t suffixCharCount = characterCount(suffix.c_str(), chineseWeight);
    if (!suffix.empty() && suffixCharCount >= maxChars) {
        return substringByChars(suffix.c_str(), maxChars, chineseWeight);
    }

    // 获取截取部分并附加后缀
    std::string result = substringByChars(str, maxChars - suffixCharCount, chineseWeight);
    result += suffix;
    return result;
}

/* ============================================================================
 * 字符集转码
** ========================================================================== */

namespace {

    /// @brief 持续转码，输出空间不足时自动扩容
    /// @param cd iconv 句柄
    /// @param inPtr 输入游标
    /// @param inBytes 剩余输入字节数
    /// @param output 输出缓冲
    /// @param outPtr 输出游标
    /// @param outLeft 输出剩余空间
    /// @return true 全部转换完成；false 出现不可恢复错误
    bool iconvAll(iconv_t cd, char*& inPtr, size_t& inBytes,
        std::string& output, char*& outPtr, size_t& outLeft) {
        for (;;) {
            if (iconv(cd, &inPtr, &inBytes, &outPtr, &outLeft) != (size_t)-1) {
                return true;
            }
            if (errno != E2BIG) {
                return false;
            }

            // 输出缓冲不足：扩容后重新定位输出游标（resize 会使旧指针失效）
            const size_t used = static_cast<size_t>(outPtr - output.data());
            output.resize(output.size() * 2 + 64);
            outPtr = &output[used];
            outLeft = output.size() - used;
        }
    }

} // namespace

std::string StringUtils::convert(const std::string& input,
    const char* fromEncoding, const char* toEncoding) {
    if (fromEncoding == nullptr || toEncoding == nullptr) return "";
    if (input.empty()) return "";

    iconv_t cd = iconv_open(toEncoding, fromEncoding);
    if (cd == (iconv_t)-1) return "";

    // 预留 4 倍空间（UTF-8 单字符最长 4 字节），不足时由 iconvAll 扩容
    std::string output(input.size() * 4 + 4, '\0');

    char* inPtr = const_cast<char*>(input.data());
    size_t inBytes = input.size();
    char* outPtr = &output[0];
    size_t outLeft = output.size();

    const bool ok = iconvAll(cd, inPtr, inBytes, output, outPtr, outLeft);
    iconv_close(cd);

    // 遇到非法字节序列（EILSEQ）或截断（EINVAL）时整串失败，与原语义一致
    if (!ok) return "";

    output.resize(static_cast<size_t>(outPtr - output.data()));
    return output;
}



