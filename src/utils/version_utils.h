/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:15:12
 * @LastEditTime: 2025-12-26 11:22:15
 * @FilePath: /kk_frame/src/utils/version_utils.h
 * @Description: 版本相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __VERSION_UTILS_H__
#define __VERSION_UTILS_H__

#include <vector>
#include <string>

namespace VersionUtils {

    /// @brief 将版本号字符串拆分为数字数组
    /// @param version 
    /// @return 
    std::vector<int> split(const std::string& version);

    /// @brief 比较两个版本号
    /// @param version1 版本号1（格式：x.x.x）
    /// @param version2 版本号2（格式：x.x.x）
    /// @return 1: version1 > version2, 0: 相等, -1: version1 < version2
    int compare(const std::string& version1, const std::string& version2);

} // VersionUtils

#endif // !__VERSION_UTILS_H__