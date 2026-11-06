/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:45
 * @LastEditTime: 2026-11-06 09:42:42
 * @FilePath: /kk_frame/src/utils/platform/file_utils.h
 * @Description: 文件系统工具（路径查询 / 读写与目录操作 / 分区容量）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include "macro_define.h"

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

/// @brief 文件系统工具：路径查询 + 读写/目录操作 + 分区容量
/// @note 分区说明：
///       1) 【不受开关影响】exists / isDir / isFile / getUsage
///          基于 stat/statvfs 的只读查询，启动路径必须可用
///       2) 【受 ENABLE_FILE_SYSTEM 控制】其余读写/删除/建目录接口；
///          关闭时统一返回 false / 空操作
namespace FileUtils {

    /* ========================================================================
     * 路径查询（只读，不依赖 ENABLE_FILE_SYSTEM）
    ** ===================================================================== */
    /// @brief 判断路径是否存在（文件或目录）
    /// @param path 路径
    /// @return true 存在
    bool exists(const std::string& path);

    /// @brief 判断路径是否为目录
    /// @param path 路径
    /// @return true 是目录
    bool isDir(const std::string& path);

    /// @brief 判断路径是否为普通文件
    /// @param path 路径
    /// @return true 是普通文件
    bool isFile(const std::string& path);

    /// @brief 分区容量信息
    struct DiskUsage {
        bool     valid{ false };  // 是否获取成功
        uint64_t total{ 0 };      // 总容量（字节）
        uint64_t available{ 0 };  // 可用容量（字节）
    };

    /// @brief 获取指定路径所在分区的容量信息
    /// @param path 路径
    /// @return 容量信息，获取失败时 valid 为 false
    DiskUsage getUsage(const std::string& path);

    /* ========================================================================
     * 文件读写与目录操作（受 ENABLE_FILE_SYSTEM 控制）
     * @note 关闭开关时全部返回 false / 空操作
    ** ===================================================================== */
    /// @brief 检测文件系统支持
    /// @return true 可用
    bool isSupported();

    /// @brief 同步写入文件
    void sync();

    /// @brief 检查文件是否存在
    /// @param filePath 文件名/路径
    /// @return true 文件存在
    bool have(const std::string& filePath);

    /// @brief 读取文件内容（二进制安全，内容原样返回）
    /// @param filePath 文件名/路径
    /// @param content 读取到的内容
    /// @return true 读取成功
    /// @note 本接口不做trim，适合二进制/JSON/配置文件
    ///       读 /proc、/sys 等需要去首尾空白的单值文件请用 SysNodeUtils::readTextFile。
    bool read(const std::string& filePath, std::string& content);

    /// @brief 写入文件内容
    /// @param filePath 文件名/路径
    /// @param content 写入的内容
    /// @return true 写入成功
    /// @note 直接覆盖目标文件；中途掉电可能造成文件残缺
    ///       配置文件等不允许残缺的场景请用 writeAtomic
    bool write(const std::string& filePath, const std::string& content);

    /// @brief 原子写入文件（临时文件 + fsync + rename + 目录 fsync）
    /// @param filePath 目标文件路径
    /// @param content 写入的内容
    /// @return true 写入成功
    /// @note 写入过程对其他读者不可见，要么是旧内容要么是完整新内容
    ///       失败时清理临时文件，不会破坏原文件
    ///       临时文件与目标同目录（保证 rename 在同一文件系统内生效），
    ///       名称为 <目标>.tmp.<pid>，避免并发写同一路径时互相踩踏
    bool writeAtomic(const std::string& filePath, const std::string& content);

    /// @brief 追加内容到文件末尾（文件不存在时创建）
    /// @param filePath 文件名/路径
    /// @param content 追加的内容
    /// @return true 追加成功
    /// @note 不在写入后 fsync，需要确保落盘时自行调用 sync()
    bool append(const std::string& filePath, const std::string& content);

    /// @brief 获取文件大小
    /// @param filePath 文件名/路径
    /// @param size 文件大小
    /// @return true 成功（文件存在且为普通文件）
    bool size(const std::string& filePath, size_t& size);

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

    /// @brief 创建目录（自动创建父目录）
    /// @param path 目录路径
    /// @return true 创建成功或目录已存在
    bool mkdir(const std::string& path);

    /// @brief 清空路径内的所有文件和子目录（不删除目录本身）
    /// @param path 目录路径
    /// @return true 清空成功
    bool clearDir(const std::string& path);

    /// @brief 递归删除目录（含目录本身）
    /// @param path 目录路径
    /// @return true 删除成功或目录本来就不存在
    /// @note 与 clearDir 的区别：本接口会一并删除目录本身
    bool removeDir(const std::string& path);

} // FileUtils

#endif // !__FILE_UTILS_H__
