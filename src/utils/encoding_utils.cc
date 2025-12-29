/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:38:27
 * @LastEditTime: 2025-12-26 11:44:12
 * @FilePath: /kk_frame/src/utils/encoding_utils.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "encoding_utils.h"
#include <iconv.h>
#include <vector>
#include <sstream>

std::string EncodingUtils::convert(const std::string& input, const char* fromEncoding, const char* toEncoding) {
    iconv_t cd = iconv_open(toEncoding, fromEncoding);
    if (cd == (iconv_t)-1)return "";

    size_t in_bytes = input.size();
    size_t out_bytes = in_bytes * 4;  // 预估输出缓冲区大小
    std::string output(out_bytes, 0);

    char* in_ptr = const_cast<char*>(input.data());
    char* out_ptr = &output[0];

    if (iconv(cd, &in_ptr, &in_bytes, &out_ptr, &out_bytes) == (size_t)-1) {
        iconv_close(cd);
        return "";
    }

    iconv_close(cd);
    output.resize(output.size() - out_bytes);
    return output;
}

std::string EncodingUtils::hexEscapes(const std::string& input) {
    std::vector<char> bytes;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && i + 3 < input.size() && input[i + 1] == 'x') {
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
