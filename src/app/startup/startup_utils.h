/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2026-07-28 14:17:17
 * @FilePath: /kk_frame/src/app/startup/startup_utils.h
 * @Description: 应用启动阶段的环境准备与信息输出
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __STARTUP_UTILS_H__
#define __STARTUP_UTILS_H__

#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdint.h>

namespace StartupUtils {

    /// @brief 初始化环境
    void env();

    /// @brief 输出项目信息
    /// @param name 
    void pInfo(int argc, const char* argv[]);

    /// @brief 输出按键映射
    void pKeyMap();

    /// @brief 创建数据目录
    void createDir();

    /// @brief 启动时间缓存服务（周期把软件时钟写入缓存文件）
    /// @param intervalMs 保存间隔（毫秒），必须大于 0
    /// @param delayMs 首次调度延迟（毫秒）
    /// @return true 已成功注册定时任务
    /// @note 必须在 TickMgr::init() 之后调用，否则注册会失败并返回 false
    bool startTimeCache(int intervalMs = 60 * 1000, int delayMs = 0);

    /// @brief 停止时间缓存服务（注销周期保存的定时任务）
    /// @return true 已成功注销定时任务；false 任务未启动或已停止
    /// @note 停止后不会立即写盘，需要时请自行调用 TimeCache::save()
    bool stopTimeCache();

    /// @brief 获取调试服务信息（按键板/电控通讯使用）
    void getDebugServiceInfo(std::string& ip, uint16_t& port);

} // StartupUtils

#endif // !__STARTUP_UTILS_H__
