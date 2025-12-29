/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:38:20
 * @LastEditTime: 2025-12-29 13:54:45
 * @FilePath: /kk_frame/src/utils/encoding_utils.h
 * @Description: 编码相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ENCODING_UTILS_H__
#define __ENCODING_UTILS_H__

#include <string>

namespace EncodingUtils {

    /// @brief 将字符串从一种编码转换为另一种编码
    /// @param input 要转换的字符串
    /// @param fromEncoding 原始编码
    /// @param toEncoding 目标编码
    /// @return 转换后的字符串
    std::string convert(const std::string& input, const char* fromEncoding, const char* toEncoding);

    /// @brief 将 "\xHH" 格式的字符串转换为实际字节流
    /// @param input 
    /// @return 
    /// @note 当看到文本显示为类似 "\xe4\xbd\xa0\xe5\xa5\xbd" 时，可以调用此函数将其转换为实际的字节流
    std::string hexEscapes(const std::string& input);

} // EncodingUtils

#endif // !__ENCODING_UTILS_H__