/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:50:48
 * @FilePath: /kk_frame/src/utils/network_utils.h
 * @Description: 网络相关的一些函数
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef KK_FRAME_UTILS_NETWORK_UTILS_H_
#define KK_FRAME_UTILS_NETWORK_UTILS_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace NetworkUtils {

/// @brief IP 地址族筛选条件
enum class AddressFamily {
    Any,  ///< IPv4 或 IPv6
    IPv4, ///< 仅 IPv4
    IPv6  ///< 仅 IPv6
};

/// @brief 主机名解析结果
struct ResolveResult {
    std::vector<std::string> addresses; ///< 不重复的文本 IP 地址
    int errorCode;                      ///< getaddrinfo 错误码，0 表示成功

    ResolveResult() : errorCode(0) { }

    /// @brief 判断解析是否成功且至少获得一个地址
    bool success() const { return errorCode == 0 && !addresses.empty(); }
};

/// @brief 网络接口状态及地址信息
/// @note IPv4/IPv6 地址分别与同下标的掩码一一对应
struct InterfaceInfo {
    std::string name;                          ///< 接口名称，例如 eth0、wlan0
    bool isUp;                                 ///< 是否具有 IFF_UP 标志
    bool isRunning;                            ///< 是否具有 IFF_RUNNING 标志
    std::vector<std::string> ipv4Addresses;    ///< IPv4 地址列表
    std::vector<std::string> ipv4Netmasks;     ///< IPv4 掩码列表
    std::vector<std::string> ipv6Addresses;    ///< IPv6 地址列表
    std::vector<std::string> ipv6Netmasks;     ///< IPv6 掩码列表
    std::string macAddress;                    ///< 大写冒号分隔 MAC 地址
    uint32_t mtu;                              ///< 最大传输单元；获取失败时为 0

    InterfaceInfo() : isUp(false), isRunning(false), mtu(0) { }
};

/// @brief 获取第一个已启用的非回环接口 IP 地址，优先返回 IPv4
/// @return 本机 IP 地址；没有可用地址时返回空字符串
std::string getIp();

/// @brief 获取指定主机解析结果中的第一个 IP 地址
/// @param host 主机名或 IP 地址
/// @return 目标 IP 地址；失败时返回空字符串。需要错误详情时使用 resolveHost
/// @note 阻塞接口：域名解析耗时由系统 DNS 配置决定，不应在 UI 或主循环线程调用
std::string getIp(const std::string& host);

/// @brief 解析主机名并返回全部不重复的地址
/// @param host 主机名或 IP 地址
/// @param family 需要返回的地址族
/// @return 解析结果及 getaddrinfo 错误码
/// @note 阻塞接口：域名解析耗时由系统 DNS 配置决定，不应在 UI 或主循环线程调用
ResolveResult resolveHost(const std::string& host,
                          AddressFamily family = AddressFamily::Any);

/// @brief 获取系统网络接口信息
/// @return 接口信息列表；读取失败时返回空列表
std::vector<InterfaceInfo> getInterfaces();

/// @brief 获取指定网络接口信息
/// @param ifname 接口名称
/// @param output 成功时写入接口信息
/// @return 找到接口返回 true
bool getInterfaceInfo(const std::string& ifname, InterfaceInfo& output);

/// @brief 判断接口是否同时具有 UP 和 RUNNING 标志
/// @param ifname 接口名称
/// @return 接口存在且处于运行状态时返回 true
bool isInterfaceUp(const std::string& ifname);

/// @brief 获取 IPv4 默认网关及其接口名
/// @param gateway 成功时写入网关 IPv4 地址
/// @param ifname 可选输出参数，成功时写入出口接口名
/// @return 找到默认网关返回 true
bool getDefaultGateway(std::string& gateway, std::string* ifname = nullptr);

/// @brief 从系统 resolver 配置获取 DNS 服务器
/// @return 不重复的 IPv4/IPv6 DNS 服务器列表
std::vector<std::string> getDnsServers();

/// @brief 判断字符串是否为合法 IPv4 地址
bool isValidIPv4(const std::string& address);

/// @brief 判断字符串是否为合法 IPv6 地址
bool isValidIPv6(const std::string& address);

/// @brief 判断字符串是否为合法 IPv4 或 IPv6 地址
bool isValidIp(const std::string& address);

/// @brief 判断端口号是否位于 1～65535
bool isValidPort(uint32_t port);

/// @brief 将文本 MAC 地址转换为 6 字节二进制地址
/// @param text 支持连续十六进制或冒号、短横线分隔格式
/// @param output 成功时写入二进制 MAC 地址
/// @return 格式合法并转换成功时返回 true
bool parseMac(const std::string& text, std::array<uint8_t, 6>& output);

/// @brief 将 6 字节二进制 MAC 地址格式化为大写冒号分隔文本
/// @param mac 二进制 MAC 地址
/// @return 格式化后的 MAC 地址
std::string formatMac(const std::array<uint8_t, 6>& mac);

/// @brief 判断字符串是否为受支持格式的 MAC 地址
bool isValidMac(const std::string& mac);

/// @brief 判断两个 IPv4 或 IPv6 地址是否位于同一子网
/// @param ip 第一个 IP 地址
/// @param peerIp 第二个 IP 地址
/// @param netmask 与地址族一致的子网掩码
/// @return 参数合法且处于同一子网时返回 true
bool isSameSubnet(const std::string& ip, const std::string& peerIp,
                  const std::string& netmask);

/// @brief 将 IPv4/IPv6 前缀长度转换为掩码
/// @param prefix IPv4 为 0～32，IPv6 为 0～128
/// @param family 地址族，不支持 AddressFamily::Any
/// @param netmask 成功时写入文本掩码
/// @return 参数合法且转换成功时返回 true
bool prefixLengthToNetmask(uint8_t prefix, AddressFamily family,
                           std::string& netmask);

/// @brief 将连续的 IPv4/IPv6 掩码转换为前缀长度
/// @param netmask IPv4 或 IPv6 掩码
/// @param prefix 成功时写入前缀长度
/// @return 掩码合法且二进制位连续时返回 true
bool netmaskToPrefixLength(const std::string& netmask, uint8_t& prefix);

/// @brief 计算 IPv4 网络地址
/// @param ip IPv4 地址
/// @param netmask IPv4 子网掩码
/// @param output 成功时写入网络地址
/// @return 参数合法且计算成功时返回 true
bool networkAddress(const std::string& ip, const std::string& netmask,
                    std::string& output);

/// @brief 计算 IPv4 广播地址
/// @param ip IPv4 地址
/// @param netmask IPv4 子网掩码
/// @param output 成功时写入广播地址
/// @return 参数合法且计算成功时返回 true
bool broadcastAddress(const std::string& ip, const std::string& netmask,
                      std::string& output);

/// @brief 判断系统是否存在 IPv4 默认路由
bool hasDefaultRoute();

/// @brief 判断指定接口是否启用且具有非回环 IP 地址
bool hasUsableAddress(const std::string& ifname);

/// @brief 判断地址是否属于 IPv4 私网或 IPv6 ULA 地址段
bool isPrivateAddress(const std::string& ip);

/// @brief 判断地址是否属于 IPv4/IPv6 回环地址段
bool isLoopbackAddress(const std::string& ip);

/// @brief 判断地址是否属于 IPv4/IPv6 组播地址段
bool isMulticastAddress(const std::string& ip);

} // namespace NetworkUtils

#endif // KK_FRAME_UTILS_NETWORK_UTILS_H_
