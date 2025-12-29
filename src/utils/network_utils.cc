/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:50:54
 * @LastEditTime: 2025-12-26 13:59:18
 * @FilePath: /kk_frame/src/utils/network_utils.cc
 * @Description: 网络相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "network_utils.h"
#include <cdlog.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <regex>

std::string NetworkUtils::getIp(const std::string& host) {
    std::string ip_addr;
    const char* hostname = host.c_str(); // 要获取IP地址的主机名
    struct addrinfo hints, * result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // 支持IPv4和IPv6
    hints.ai_socktype = SOCK_STREAM; // 使用TCP协议

    // 解析主机名
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return "";
    }

    // 遍历结果列表并打印IP地址
    struct addrinfo* p = result;
    while (p != NULL) {
        void* addr;
        if (p->ai_family == AF_INET) { // IPv4地址
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6地址
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        char ip[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ip, sizeof(ip));
        ip_addr = ip;
        break;

        p = p->ai_next;
    }

    freeaddrinfo(result);
    return ip_addr;
}

int NetworkUtils::wifiSignal(int SignalLevel) {
    if (SignalLevel > -60) return 4;
    else if (SignalLevel > -75) return 3;
    else if (SignalLevel > -90) return 2;
    else return 1;
}

int NetworkUtils::wifiSignal(float Quality, int SignalLevel) {
    if (Quality > 0.75 && SignalLevel > -50) return 4;
    else if (Quality > 0.5 && SignalLevel > -60) return 3;
    else if (Quality > 0.25 && SignalLevel > -70) return 2;
    else if (Quality < 0.25 || SignalLevel < -70) return 1;
    else return 1;
}

bool NetworkUtils::macToChar(const char* mac, char* macChar) {
    if (!isValidMac(mac)) {
        LOGE("Invalid MAC address");
        return false;
    }
    std::istringstream ss(mac);
    std::string item;
    for (int i = 0; i < 6; ++i) {
        if (std::getline(ss, item, ':')) {
            macChar[i] = static_cast<uint8_t>(std::stoi(item, nullptr, 16));
        } else {
            LOGE("Invalid MAC address format.");
            return false;
        }
    }
    return true;
}

bool NetworkUtils::isValidMac(const char* mac) {
    std::regex macRegex(R"((([0-9A-Fa-f]{2}[-:]){5}([0-9A-Fa-f]{2}))|(([0-9A-Fa-f]{2}){6}))");
    return std::regex_match(mac, macRegex);
}
