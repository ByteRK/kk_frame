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

#include "quick_define.h"

#include <string>
#include <vector>
#include <functional>

/// @note 本模块受 ENABLE_FILE_SYSTEM 宏控制：
///       开关关闭时所有接口均不可用（返回 false / 空操作），调用方无需改动即可编译。
///       可用 FileUtils::isSupported() 在运行时判断当前能力。
namespace FileUtils {

    /// @brief 检测文件系统支持。由 ENABLE_FILE_SYSTEM 宏控制。
    /// @return true 可用；false 时本模块所有接口均返回失败
    bool isSupported();

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

    /// @brief 删除文件
    /// @param filePath 文件名/路径
    /// @return true 删除成功或文件本来就不存在
    /// @note 仅删除普通文件，不会删除目录
    bool remove(const std::string& filePath);

    /// @brief 删除主/备文件并同步
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径，可传空
    /// @return true 全部删除成功或文件本来就不存在
    /// @note 两个文件都会尝试删除，不会因主文件失败而跳过备份文件
    bool remove(const std::string& filePath, const std::string& backupPath);

    /// @brief 遍历判断文件是否存在并根据回调检测文件是否正常
    /// @param fileList 文件路径列表
    /// @param callback 回调处理函数
    /// @return 处理结果
    /// @note 用于配置文件加载，传入项为文件路径以及备份路径
    bool check(const std::vector<std::string>& fileList, std::function<bool(const std::string&, size_t)> callback);

    /// @brief 判断路径是否为目录
    /// @param path 路径
    /// @return true 是目录
    bool isDir(const std::string& path);

    /// @brief 创建目录（自动创建父目录）
    /// @param path 目录路径
    /// @return true 创建成功或目录已存在
    bool mkdir(const std::string& path);

    /// @brief 清空路径内的所有文件和子目录（不删除目录本身）
    /// @param path 目录路径
    /// @return true 清空成功
    bool clearDir(const std::string& path);

} // FileUtils

#endif // !__FILE_UTILS_H__
