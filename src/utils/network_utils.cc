/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:50:54
 * @FilePath: /kk_frame/src/utils/network_utils.cc
 * @Description: 网络相关的一些函数
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "network_utils.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netpacket/packet.h>
#include <set>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int toNativeFamily(NetworkUtils::AddressFamily family) {
    if (family == NetworkUtils::AddressFamily::IPv4) return AF_INET;
    if (family == NetworkUtils::AddressFamily::IPv6) return AF_INET6;
    return AF_UNSPEC;
}

bool sockaddrToString(const sockaddr* address, std::string& output) {
    if (address == nullptr) return false;

    const void* source = nullptr;
    socklen_t size = 0;
    if (address->sa_family == AF_INET) {
        source = &reinterpret_cast<const sockaddr_in*>(address)->sin_addr;
        size = INET_ADDRSTRLEN;
    } else if (address->sa_family == AF_INET6) {
        source = &reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr;
        size = INET6_ADDRSTRLEN;
    } else {
        return false;
    }

    char buffer[INET6_ADDRSTRLEN] = { 0 };
    if (::inet_ntop(address->sa_family, source, buffer, size) == nullptr) {
        return false;
    }
    output.assign(buffer);
    return true;
}

NetworkUtils::InterfaceInfo* findInterface(
        std::vector<NetworkUtils::InterfaceInfo>& interfaces,
        const std::string& name) {
    for (size_t i = 0; i < interfaces.size(); ++i) {
        if (interfaces[i].name == name) return &interfaces[i];
    }
    interfaces.push_back(NetworkUtils::InterfaceInfo());
    interfaces.back().name = name;
    return &interfaces.back();
}

uint32_t getMtu(const std::string& ifname) {
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return 0;

    struct ifreq request;
    std::memset(&request, 0, sizeof(request));
    std::strncpy(request.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    const bool ok = ::ioctl(fd, SIOCGIFMTU, &request) == 0;
    ::close(fd);
    return ok && request.ifr_mtu > 0
        ? static_cast<uint32_t>(request.ifr_mtu) : 0;
}

bool parseAddress(const std::string& text, int family,
                  std::array<uint8_t, 16>& bytes, size_t& size) {
    bytes.fill(0);
    size = family == AF_INET ? 4U : 16U;
    return ::inet_pton(family, text.c_str(), bytes.data()) == 1;
}

bool formatAddress(const uint8_t* bytes, int family, std::string& output) {
    char buffer[INET6_ADDRSTRLEN] = { 0 };
    if (::inet_ntop(family, bytes, buffer, sizeof(buffer)) == nullptr) return false;
    output.assign(buffer);
    return true;
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool parseSubnetInputs(const std::string& ip, const std::string& netmask,
                       std::array<uint8_t, 16>& ipBytes,
                       std::array<uint8_t, 16>& maskBytes,
                       int& family, size_t& size) {
    family = NetworkUtils::isValidIPv4(ip) ? AF_INET : AF_INET6;
    size_t ipSize = 0;
    size_t maskSize = 0;
    return parseAddress(ip, family, ipBytes, ipSize)
        && parseAddress(netmask, family, maskBytes, maskSize)
        && ipSize == maskSize && (size = ipSize) > 0;
}

} // namespace

std::string NetworkUtils::getIp() {
    const std::vector<InterfaceInfo> interfaces = getInterfaces();
    for (size_t pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < interfaces.size(); ++i) {
            const InterfaceInfo& info = interfaces[i];
            if (!info.isUp || info.name == "lo") continue;
            const std::vector<std::string>& addresses =
                pass == 0 ? info.ipv4Addresses : info.ipv6Addresses;
            for (size_t j = 0; j < addresses.size(); ++j) {
                if (!isLoopbackAddress(addresses[j])) return addresses[j];
            }
        }
    }
    return std::string();
}

std::string NetworkUtils::getIp(const std::string& host) {
    const ResolveResult result = resolveHost(host);
    return result.addresses.empty() ? std::string() : result.addresses.front();
}

NetworkUtils::ResolveResult NetworkUtils::resolveHost(
        const std::string& host, AddressFamily family) {
    ResolveResult output;
    if (host.empty()) {
        output.errorCode = EAI_NONAME;
        return output;
    }

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = toNativeFamily(family);
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    output.errorCode = ::getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (output.errorCode != 0) return output;

    std::set<std::string> seen;
    for (const struct addrinfo* item = result; item != nullptr; item = item->ai_next) {
        std::string address;
        if (sockaddrToString(item->ai_addr, address) && seen.insert(address).second) {
            output.addresses.push_back(address);
        }
    }
    ::freeaddrinfo(result);
    if (output.addresses.empty()) output.errorCode = EAI_NONAME;
    return output;
}

std::vector<NetworkUtils::InterfaceInfo> NetworkUtils::getInterfaces() {
    std::vector<InterfaceInfo> output;
    struct ifaddrs* list = nullptr;
    if (::getifaddrs(&list) != 0) return output;

    for (const struct ifaddrs* item = list; item != nullptr; item = item->ifa_next) {
        if (item->ifa_name == nullptr) continue;
        InterfaceInfo* info = findInterface(output, item->ifa_name);
        info->isUp = (item->ifa_flags & IFF_UP) != 0;
        info->isRunning = (item->ifa_flags & IFF_RUNNING) != 0;
        if (item->ifa_addr == nullptr) continue;

        std::string address;
        if (item->ifa_addr->sa_family == AF_INET
                || item->ifa_addr->sa_family == AF_INET6) {
            if (sockaddrToString(item->ifa_addr, address)) {
                std::vector<std::string>& addresses =
                    item->ifa_addr->sa_family == AF_INET
                        ? info->ipv4Addresses : info->ipv6Addresses;
                if (std::find(addresses.begin(), addresses.end(), address)
                        == addresses.end()) {
                    addresses.push_back(address);
                    std::string netmask;
                    sockaddrToString(item->ifa_netmask, netmask);
                    std::vector<std::string>& netmasks =
                        item->ifa_addr->sa_family == AF_INET
                            ? info->ipv4Netmasks : info->ipv6Netmasks;
                    netmasks.push_back(netmask);
                }
            }
        } else if (item->ifa_addr->sa_family == AF_PACKET) {
            const sockaddr_ll* link =
                reinterpret_cast<const sockaddr_ll*>(item->ifa_addr);
            if (link->sll_halen == 6) {
                std::array<uint8_t, 6> mac;
                std::copy(link->sll_addr, link->sll_addr + 6, mac.begin());
                info->macAddress = formatMac(mac);
            }
        }
    }
    ::freeifaddrs(list);

    for (size_t i = 0; i < output.size(); ++i) {
        output[i].mtu = getMtu(output[i].name);
    }
    return output;
}

bool NetworkUtils::getInterfaceInfo(const std::string& ifname,
                                    InterfaceInfo& output) {
    const std::vector<InterfaceInfo> interfaces = getInterfaces();
    for (size_t i = 0; i < interfaces.size(); ++i) {
        if (interfaces[i].name == ifname) {
            output = interfaces[i];
            return true;
        }
    }
    return false;
}

bool NetworkUtils::isInterfaceUp(const std::string& ifname) {
    InterfaceInfo info;
    return getInterfaceInfo(ifname, info) && info.isUp && info.isRunning;
}

bool NetworkUtils::getDefaultGateway(std::string& gateway, std::string* ifname) {
    gateway.clear();
    if (ifname != nullptr) ifname->clear();
    std::ifstream routes("/proc/net/route");
    if (!routes) return false;

    std::string line;
    std::getline(routes, line);
    while (std::getline(routes, line)) {
        std::istringstream stream(line);
        std::string interfaceName;
        std::string destination;
        std::string gatewayHex;
        unsigned int flags = 0;
        if (!(stream >> interfaceName >> destination >> gatewayHex
                    >> std::hex >> flags)) continue;
        if (destination != "00000000" || (flags & 0x2U) == 0) continue;

        if (gatewayHex.size() != 8) continue;
        uint8_t bytes[4] = { 0 };
        bool valid = true;
        for (size_t i = 0; i < 4; ++i) {
            const size_t offset = (3 - i) * 2;
            const int high = hexValue(gatewayHex[offset]);
            const int low = hexValue(gatewayHex[offset + 1]);
            if (high < 0 || low < 0) {
                valid = false;
                break;
            }
            bytes[i] = static_cast<uint8_t>((high << 4) | low);
        }
        if (!valid) continue;
        struct in_addr address;
        std::memcpy(&address.s_addr, bytes, sizeof(bytes));
        char buffer[INET_ADDRSTRLEN] = { 0 };
        if (::inet_ntop(AF_INET, &address, buffer, sizeof(buffer)) == nullptr) continue;
        gateway.assign(buffer);
        if (ifname != nullptr) *ifname = interfaceName;
        return true;
    }
    return false;
}

std::vector<std::string> NetworkUtils::getDnsServers() {
    std::vector<std::string> output;
    std::ifstream resolver("/etc/resolv.conf");
    std::string line;
    while (std::getline(resolver, line)) {
        std::istringstream stream(line);
        std::string keyword;
        std::string address;
        if (!(stream >> keyword >> address) || keyword != "nameserver"
                || !isValidIp(address)) continue;
        if (std::find(output.begin(), output.end(), address) == output.end()) {
            output.push_back(address);
        }
    }
    return output;
}

bool NetworkUtils::isValidIPv4(const std::string& address) {
    struct in_addr value;
    return ::inet_pton(AF_INET, address.c_str(), &value) == 1;
}

bool NetworkUtils::isValidIPv6(const std::string& address) {
    struct in6_addr value;
    return ::inet_pton(AF_INET6, address.c_str(), &value) == 1;
}

bool NetworkUtils::isValidIp(const std::string& address) {
    return isValidIPv4(address) || isValidIPv6(address);
}

bool NetworkUtils::isValidPort(uint32_t port) {
    return port > 0 && port <= 65535U;
}

bool NetworkUtils::parseMac(const std::string& text,
                            std::array<uint8_t, 6>& output) {
    std::string compact;
    compact.reserve(12);
    char separator = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        const char value = text[i];
        if (value == ':' || value == '-') {
            if (separator == 0) separator = value;
            if (separator != value || i == 0 || i + 1 == text.size()
                    || text[i - 1] == separator) return false;
        } else if (hexValue(value) >= 0) {
            compact.push_back(value);
        } else {
            return false;
        }
    }
    if (compact.size() != 12) return false;
    if (separator != 0) {
        if (text.size() != 17) return false;
        for (size_t i = 2; i < text.size(); i += 3) {
            if (text[i] != separator) return false;
        }
    }
    for (size_t i = 0; i < output.size(); ++i) {
        output[i] = static_cast<uint8_t>(
            (hexValue(compact[i * 2]) << 4) | hexValue(compact[i * 2 + 1]));
    }
    return true;
}

std::string NetworkUtils::formatMac(const std::array<uint8_t, 6>& mac) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (size_t i = 0; i < mac.size(); ++i) {
        if (i != 0) stream << ':';
        stream << std::setw(2) << static_cast<unsigned int>(mac[i]);
    }
    return stream.str();
}

bool NetworkUtils::isValidMac(const std::string& mac) {
    std::array<uint8_t, 6> ignored;
    return parseMac(mac, ignored);
}

bool NetworkUtils::isSameSubnet(const std::string& ip,
                                const std::string& peerIp,
                                const std::string& netmask) {
    std::array<uint8_t, 16> first;
    std::array<uint8_t, 16> mask;
    int family = AF_UNSPEC;
    size_t size = 0;
    if (!parseSubnetInputs(ip, netmask, first, mask, family, size)) return false;
    std::array<uint8_t, 16> second;
    size_t secondSize = 0;
    if (!parseAddress(peerIp, family, second, secondSize) || size != secondSize) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if ((first[i] & mask[i]) != (second[i] & mask[i])) return false;
    }
    return true;
}

bool NetworkUtils::prefixLengthToNetmask(uint8_t prefix, AddressFamily family,
                                         std::string& netmask) {
    const size_t size = family == AddressFamily::IPv4 ? 4U
        : family == AddressFamily::IPv6 ? 16U : 0U;
    if (size == 0 || prefix > size * 8) return false;
    std::array<uint8_t, 16> bytes;
    bytes.fill(0);
    for (size_t bit = 0; bit < prefix; ++bit) {
        bytes[bit / 8] |= static_cast<uint8_t>(0x80U >> (bit % 8));
    }
    return formatAddress(bytes.data(), size == 4 ? AF_INET : AF_INET6, netmask);
}

bool NetworkUtils::netmaskToPrefixLength(const std::string& netmask,
                                         uint8_t& prefix) {
    std::array<uint8_t, 16> bytes;
    size_t size = 0;
    const int family = isValidIPv4(netmask) ? AF_INET : AF_INET6;
    if (!parseAddress(netmask, family, bytes, size)) return false;
    unsigned int count = 0;
    bool foundZero = false;
    for (size_t i = 0; i < size; ++i) {
        for (unsigned int bit = 0; bit < 8; ++bit) {
            const bool set = (bytes[i] & (0x80U >> bit)) != 0;
            if (!set) foundZero = true;
            else if (foundZero) return false;
            else ++count;
        }
    }
    prefix = static_cast<uint8_t>(count);
    return true;
}

bool NetworkUtils::networkAddress(const std::string& ip,
                                  const std::string& netmask,
                                  std::string& output) {
    std::array<uint8_t, 16> address;
    std::array<uint8_t, 16> mask;
    int family = AF_UNSPEC;
    size_t size = 0;
    if (!parseSubnetInputs(ip, netmask, address, mask, family, size)
            || family != AF_INET) return false;
    for (size_t i = 0; i < size; ++i) address[i] &= mask[i];
    return formatAddress(address.data(), family, output);
}

bool NetworkUtils::broadcastAddress(const std::string& ip,
                                    const std::string& netmask,
                                    std::string& output) {
    std::array<uint8_t, 16> address;
    std::array<uint8_t, 16> mask;
    int family = AF_UNSPEC;
    size_t size = 0;
    if (!parseSubnetInputs(ip, netmask, address, mask, family, size)
            || family != AF_INET) return false;
    for (size_t i = 0; i < size; ++i) {
        address[i] = static_cast<uint8_t>(address[i] | ~mask[i]);
    }
    return formatAddress(address.data(), family, output);
}

bool NetworkUtils::hasDefaultRoute() {
    std::string gateway;
    return getDefaultGateway(gateway);
}

bool NetworkUtils::hasUsableAddress(const std::string& ifname) {
    InterfaceInfo info;
    if (!getInterfaceInfo(ifname, info) || !info.isUp) return false;
    for (size_t i = 0; i < info.ipv4Addresses.size(); ++i) {
        if (info.ipv4Addresses[i] != "0.0.0.0"
                && !isLoopbackAddress(info.ipv4Addresses[i])) return true;
    }
    for (size_t i = 0; i < info.ipv6Addresses.size(); ++i) {
        if (info.ipv6Addresses[i] != "::"
                && !isLoopbackAddress(info.ipv6Addresses[i])) return true;
    }
    return false;
}

bool NetworkUtils::isPrivateAddress(const std::string& ip) {
    struct in_addr ipv4;
    if (::inet_pton(AF_INET, ip.c_str(), &ipv4) == 1) {
        const uint32_t value = ntohl(ipv4.s_addr);
        return (value & 0xff000000U) == 0x0a000000U
            || (value & 0xfff00000U) == 0xac100000U
            || (value & 0xffff0000U) == 0xc0a80000U;
    }
    struct in6_addr ipv6;
    if (::inet_pton(AF_INET6, ip.c_str(), &ipv6) == 1) {
        return (ipv6.s6_addr[0] & 0xfeU) == 0xfcU;
    }
    return false;
}

bool NetworkUtils::isLoopbackAddress(const std::string& ip) {
    struct in_addr ipv4;
    if (::inet_pton(AF_INET, ip.c_str(), &ipv4) == 1) {
        return (ntohl(ipv4.s_addr) & 0xff000000U) == 0x7f000000U;
    }
    struct in6_addr ipv6;
    return ::inet_pton(AF_INET6, ip.c_str(), &ipv6) == 1
        && IN6_IS_ADDR_LOOPBACK(&ipv6);
}

bool NetworkUtils::isMulticastAddress(const std::string& ip) {
    struct in_addr ipv4;
    if (::inet_pton(AF_INET, ip.c_str(), &ipv4) == 1) {
        return (ntohl(ipv4.s_addr) & 0xf0000000U) == 0xe0000000U;
    }
    struct in6_addr ipv6;
    return ::inet_pton(AF_INET6, ip.c_str(), &ipv6) == 1
        && IN6_IS_ADDR_MULTICAST(&ipv6);
}
