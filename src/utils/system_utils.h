/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:21
 * @LastEditTime: 2026-06-16 23:45:12
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

    /// @brief 结束程序
    void exit();

    /// @brief 数据同步
    /// @note 将内存数据同步到储存
    void sync();
    
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

    /// @brief 设置系统时间
    /// @param timestamp 秒级时间戳
    void setTime(const int64_t& timestamp);

    /// @brief 设置系统时间，参数小于 0 表示保持当前值
    /// @param year 年，大于 0 时生效
    /// @param month 月，1-12 时生效
    /// @param day 日，大于 0 时生效
    /// @param hour 时，0-23 时生效
    /// @param minute 分，0-59 时生效
    /// @param second 秒，0-59 时生效
    void setTime(int year, int month, int day, int hour, int minute, int second = 0);

    /// @brief 同步硬件时钟
    void syncHWClock();

    /// @brief 获取德明利TP版本号
    /// @return 版本号
    int getTwscTpVersion();

} // SystemUtils

#endif // !__SYSTEM_UTILS_H__
