/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:38:27
 * @LastEditTime: 2026-11-06 10:00:00
 * @FilePath: /kk_frame/src/utils/base/bytes_utils.cc
 * @Description: 字节级工具（线路字节序读写、十六进制编解码）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "bytes_utils.h"

#include <cctype>
#include <sstream>
#include <vector>

std::string BytesUtils::hexEscapes(const std::string& input) {
    std::vector<char> bytes;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && i + 3 < input.size() && input[i + 1] == 'x'
            && std::isxdigit(static_cast<unsigned char>(input[i + 2]))
            && std::isxdigit(static_cast<unsigned char>(input[i + 3]))) {
            // 提取十六进制值（如 "e4"）
            std::string hex_str = input.substr(i + 2, 2);
            std::istringstream hex_stream(hex_str);
            int byte_value;
            if (hex_stream >> std::hex >> byte_value) {
                bytes.push_back(static_cast<char>(byte_value));
                i += 4; // 跳过已处理的 "\xHH"
            } else {
                // 解析失败，保留原字符
                bytes.push_back(input[i++]);
            }
        } else {
            bytes.push_back(input[i++]);
        }
    }
    return std::string(bytes.begin(), bytes.end());
}

std::string BytesUtils::hexEncode(const void* data, size_t length) {
    static const char kHex[] = "0123456789abcdef";
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    if (bytes == nullptr || length == 0) return std::string();

    std::string result;
    result.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(kHex[bytes[i] >> 4]);
        result.push_back(kHex[bytes[i] & 0x0f]);
    }
    return result;
}

std::string BytesUtils::hexEncode(const std::string& data) {
    return hexEncode(data.data(), data.size());
}

bool BytesUtils::hexDecode(const std::string& hex, std::string& out, bool allowSeparators) {
    std::string result;
    result.reserve(hex.size() / 2);

    int high = -1;
    for (size_t i = 0; i < hex.size(); ++i) {
        const char ch = hex[i];
        if (allowSeparators && (ch == ' ' || ch == ':' || ch == '-' || ch == '\t')) {
            continue;
        }

        const int value = hexValue(ch);
        if (value < 0) {
            return false;
        }

        if (high < 0) {
            high = value;
        } else {
            result.push_back(static_cast<char>((high << 4) | value));
            high = -1;
        }
    }

    // 存在落单的半个字节
    if (high >= 0) {
        return false;
    }

    out.swap(result);
    return true;
}
