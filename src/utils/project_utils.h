/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2026-07-05 23:27:16
 * @FilePath: /kk_frame/src/utils/project_utils.h
 * @Description: 项目相关的一些操作函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PROJECT_UTILS_H__
#define __PROJECT_UTILS_H__

#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdint.h>

namespace ProjectUtils {

    /// @brief 初始化环境
    void env();

    /// @brief 输出项目信息
    /// @param name 
    void pInfo(int argc, const char* argv[]);

    /// @brief 输出按键映射
    void pKeyMap();

    /// @brief 获取调试串口信息
    void getDebugServiceInfo(std::string& ip, uint16_t& port);

    /// @brief 保存当前时间到本地文件
    void saveTime(const std::string& filename);

    /// @brief 从本地文件加载时间
    /// @param filename 
    void loadTime(const std::string& filename);

} // ProjectUtils

#endif // !__PROJECT_UTILS_H__
