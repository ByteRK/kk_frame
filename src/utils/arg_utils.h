/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-29 01:21:48
 * @LastEditTime: 2026-04-22 16:56:47
 * @FilePath: /kk_frame/src/utils/arg_utils.h
 * @Description: 命令行参数解析工具
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ARG_UTILS_H__
#define __ARG_UTILS_H__

#include <string>
#include <vector>

namespace ArgUtils {

    // 解析结果结构体
    struct ArgValues {
        bool isDemo;
        int  selectPage;
        bool version;
    };

    /// @brief 获取解析结果
    /// @return 只读解析结果结构体
    const ArgValues& get();

    /// @brief 获取原始命令行参数
    /// @return 只读原始命令行参数列表
    const std::vector<std::string>& getRaw();

    /// @brief 获取拼接后的完整命令行参数字符串
    /// @return 只读完整命令行参数字符串
    const std::string& getRawString();
    
    /// @brief 命令行参数解析
    /// @param argc 命令行参数个数
    /// @param argv 命令行参数数组
    /// @return 解析后的参数值结构体
    void parse(int argc, const char* argv[]);

} // namespace ArgUtils

#endif // !__ARG_UTILS_H__
