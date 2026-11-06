/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-04 12:00:00
 * @FilePath: /kk_frame/src/utils/format_utils.cc
 * @Description: 展示用格式化（十六进制、容量单位）相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "format_utils.h"
#include "string_utils.h"

#include <cdlog.h>

std::string FormatUtils::hexStr(const unsigned char* buf, int len, int width) {
    // 保持与原 std::hex 输出一致：小写字母
    static const char kHexDigits[] = "0123456789abcdef";

    if (buf == nullptr || len <= 0) return std::string();

    // 每个字节 2 位十六进制 + 1 个空格，另加换行
    const size_t lineCount = (width > 0) ? static_cast<size_t>(len / width) : 0;
    std::string result;
    result.reserve(static_cast<size_t>(len) * 3 + lineCount);

    for (int i = 0; i < len; i++) {
        const unsigned char byte = buf[i];
        result.push_back(kHexDigits[byte >> 4]);
        result.push_back(kHexDigits[byte & 0x0F]);
        result.push_back(' ');
        if (width > 0 && (i + 1) % width == 0) {
            result.push_back('\n');
        }
    }
    return result;
}

void FormatUtils::hexdump(const char* label, const unsigned char* buf, int len, int width) {
    if (label == nullptr || *label == '\0') {
        LOGD("hex dump:\n%s", hexStr(buf, len, width).c_str());
    } else {
        LOGD("%s hex dump:\n%s", label, hexStr(buf, len, width).c_str());
    }
}

std::string FormatUtils::sizeVisualization(size_t byteSize, int precision, bool includeTB) {
    static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    const int maxUnit = includeTB ? 4 : 3;

    double size = static_cast<double>(byteSize);
    int unitIndex = 0;
    while (size >= 1024.0 && unitIndex < maxUnit) {
        size /= 1024.0;
        ++unitIndex;
    }

    if (precision < 0) precision = 0;
    const int decimals = (unitIndex == 0) ? 0 : precision;
    return StringUtils::format("%.*f %s", decimals, size, units[unitIndex]);
}
