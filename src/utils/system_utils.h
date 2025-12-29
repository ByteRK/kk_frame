/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:21
 * @LastEditTime: 2025-12-26 18:27:09
 * @FilePath: /kk_frame/src/utils/system_utils.h
 * @Description: 系统相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __SYSTEM_UTILS_H__
#define __SYSTEM_UTILS_H__

#include <string>

namespace SystemUtils {

    /// @brief 重启
    void reboot();
    
    /// @brief 执行系统命令并获取结果
    /// @param cmd 命令
    /// @return 执行结果
    std::string system(const std::string& cmd);

    /// @brief 异步执行系统命令
    /// @param cmd 命令
    /// @return true 成功
    bool asyncSystem(const std::string& cmd);

    /// @brief 写入值到系统路径
    /// @param path 路径
    /// @param value 值
    /// @note 可替代echo，节约消耗
    bool sysfs(const std::string& path, const std::string& value);

    /// @brief 设置系统亮度
    /// @param value 亮度值 [0-100]
    /// @param swap 是否反转亮度值
    void setBrightness(int value, bool swap = false);

} // SystemUtils

#endif // !__SYSTEM_UTILS_H__
