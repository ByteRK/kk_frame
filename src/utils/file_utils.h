/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:45
 * @LastEditTime: 2025-12-26 14:38:44
 * @FilePath: /kk_frame/src/utils/file_utils.h
 * @Description: 文件相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include <string>

namespace FileUtils {

    /// @brief 同步写入文件
    void sync();

    /// @brief 读取文件内容
    /// @param filePath 文件名/路径
    /// @param content 读取到的内容
    /// @return true 读取成功
    bool read(const std::string& filePath, std::string& content);

    /// @brief 写入文件内容
    /// @param fileName 文件名/路径
    /// @param content 写入的内容
    /// @return true 写入成功
    bool write(const std::string& filePath, const std::string& content);

    /// @brief 检查文件是否存在并返回文件大小
    /// @param filePath 文件名/路径
    /// @param size 文件大小
    /// @return true 文件存在
    bool check(const std::string& filePath, size_t& size);
    
} // FileUtils

#endif // !__FILE_UTILS_H__
