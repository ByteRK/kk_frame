/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:50:48
 * @LastEditTime: 2025-12-26 14:00:20
 * @FilePath: /kk_frame/src/utils/network_utils.h
 * @Description: 网络相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __NETWORK_UTILS_H__
#define __NETWORK_UTILS_H__

#include <string>

namespace NetworkUtils {

    /// @brief 获取本机IP地址
    /// @return 本机IP地址
    std::string getIp();

    /// @brief 获取指定主机的IP地址
    /// @param host 主机地址
    /// @return 目标IP地址
    std::string getIp(const std::string& host);

    /// @brief 根据信号强度计算WIFI信号强度
    /// @param SignalLevel 信号强度 dBm
    /// @return 计算后的WIFI信号强度(1-4)
    int wifiSignal(int SignalLevel);

    /// @brief 根据信号强度和信号质量计算wifi信号强度
    /// @param Quality 信号质量 %
    /// @param SignalLevel 信号强度 dBm
    /// @return 计算后的WIFI信号强度(1-4)
    int wifiSignal(float Quality, int SignalLevel);

    /// @brief 将MAC地址转换为十六进制
    /// @param mac MAC地址
    /// @param macChar 字符串
    /// @return 是否转换成功
    /// @note 预分配的macChar空间必须大于等于6
    bool macToChar(const char* mac, char* macChar);

    /// @brief 判断MAC地址是否合法
    /// @param mac 
    /// @return 
    bool isValidMac(const char* mac);

} // NetworkUtils

#endif // !__NETWORK_UTILS_H__