/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-06 09:36:57
 * @FilePath: /kk_frame/src/utils/base/path_utils.h
 * @Description: 路径字符串处理相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PATH_UTILS_H__
#define __PATH_UTILS_H__

#include <string>

/// @brief 路径字符串处理相关的一些函数
namespace PathUtils {

    /// @brief 路径分隔符
    constexpr char separator = '/';

    /// @brief 判断路径是否以分隔符结尾
    /// @param path 路径
    /// @return true 以分隔符结尾
    bool hasTrailingSeparator(const std::string& path);

    /// @brief 确保路径以分隔符结尾
    /// @param path 路径
    /// @return 以分隔符结尾的路径；空路径原样返回
    std::string ensureTrailingSeparator(const std::string& path);

    /// @brief 去掉路径结尾多余的的分隔符
    /// @param path 路径
    /// @return 处理后的路径；根路径 "/" 保持原样
    std::string removeTrailingSeparator(const std::string& path);

    /// @brief 拼接目录与名称
    /// @param dir 目录
    /// @param name 名称或子路径
    /// @return 拼接结果（自动处理分隔符）；name 为绝对路径时直接返回 name
    std::string join(const std::string& dir, const std::string& name);

    /// @brief 判断是否为绝对路径
    bool isAbsolute(const std::string& path);

    /// @brief 取路径最后一段（文件名，含扩展名）
    /// @param path 路径
    /// @return 最后一段；路径为空时返回空字符串
    /// @note 结尾的分隔符会被忽略（"/a/b/" -> "b"），与 dirName 的归一化行为一致
    std::string fileName(const std::string& path);

    /// @brief 取路径最后一段并去掉扩展名
    /// @param path 路径
    /// @return 不含扩展名的名称
    std::string baseName(const std::string& path);

    /// @brief 取扩展名
    /// @param path 路径
    /// @return 含点的扩展名（如 ".txt"）；无扩展名返回空字符串
    std::string extension(const std::string& path);

    /// @brief 取父目录路径
    /// @param path 路径
    /// @return 父目录；无父目录返回空字符串，根目录返回 "/"
    std::string dirName(const std::string& path);

    /// @brief 归一化路径
    /// @param path 路径
    /// @return 合并重复分隔符、消解 "." 与 ".." 后的路径
    /// @note 纯字符串处理，不访问磁盘，也不处理软链接
    std::string normalize(const std::string& path);

    /// @brief 判断 child 是否位于 parent 目录之内
    /// @param parent 父目录
    /// @param child 子路径
    /// @return true 在目录之内（相等也视为在内）
    /// @note 两侧都会先归一化，因此 "a/../../b" 这类序列不会绕过判断；
    ///       纯字符串判断，不解析软链接，安全敏感场景请再配合 realpath
    bool isSubPath(const std::string& parent, const std::string& child);

} // PathUtils

#endif // !__PATH_UTILS_H__
