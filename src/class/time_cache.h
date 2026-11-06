/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-06 09:31:42
 * @FilePath: /kk_frame/src/class/time_cache.h
 * @Description: 时间缓存文件读写（用于重启前的软件时钟保值）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TIME_CACHE_H__
#define __TIME_CACHE_H__

#include <ctime>
#include <string>

/// @brief 时间缓存文件（重启前保存、上电后恢复）
namespace TimeCache {

    /// @brief 设置缓存文件路径
    /// @param path 缓存文件完整路径
    /// @note 必须在 save/read/clear 之前调用；未设置时相关接口直接返回失败
    void setPath(const std::string& path);

    /// @brief 获取缓存文件路径
    /// @return 形如 "/xxx/data/timeCache"；未设置时返回空字符串
    const std::string& path();

    /// @brief 缓存文件路径是否已设置
    /// @return true 已设置
    bool isReady();

    /// @brief 将当前时间写入缓存文件
    /// @return true 写入成功
    bool save();

    /// @brief 从缓存文件读取时间戳
    /// @param timestamp 成功时写入秒级时间戳
    /// @return true 读取成功且时间戳有效
    bool read(time_t& timestamp);

    /// @brief 恢复系统时间（读取缓存并写入系统时钟）
    /// @return true 读取与设置系统时钟均成功
    /// @note 无权限或平台不支持设置时钟时返回 false，调用方可据此决定是否重试
    bool recover();

    /// @brief 删除缓存文件
    /// @return true 删除成功或文件本来就不存在
    bool clear();

} // TimeCache

#endif // !__TIME_CACHE_H__
