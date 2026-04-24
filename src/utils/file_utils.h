/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:45
 * @LastEditTime: 2026-04-24 16:45:16
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
#include <vector>
#include <functional>

namespace FileUtils {

    /// @brief 同步写入文件
    void sync();

    /// @brief 检查文件是否存在
    /// @param filePath 文件名/路径
    /// @return true 文件存在
    bool have(const std::string& filePath);

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
    bool check(const std::string& filePath, size_t* size = nullptr);

    /// @brief 遍历判断文件是否存在并根据回调检测文件是否正常
    /// @param fileList 文件路径列表
    /// @param callback 回调处理函数
    /// @return 处理结果
    /// @note 用于配置文件加载，传入项为文件路径以及备份路径
    bool check(const std::vector<std::string>& fileList, std::function<bool(const std::string&, size_t)> callback);

} // FileUtils

#endif // !__FILE_UTILS_H__
