/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_network.cc
 * @Description: 网络信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_network.h"

#include "network_utils.h"
#include "string_utils.h"

FactoryNetwork::FactoryNetwork(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_network") {
    setExitBtn(AppRid::exit);
    setInterface();
    PBase::click(mRootView, AppRid::refresh, [this](View&) {setInterface();});
}

void FactoryNetwork::setInterface() {
    TextView* interFace = PBase::get<TextView>(mRootView, AppRid::inter_face);
    std::string info;

    // 网络信息
    std::vector<NetworkUtils::InterfaceInfo> netInfo = NetworkUtils::getInterfaces();
    if (netInfo.empty()) {
        info = "未检测到网络接口\n";
    }

    for (const auto& item : netInfo) {
        info += StringUtils::format("[%s]  %s / %s\n",
            item.name.c_str(),
            item.isUp ? "UP" : "DOWN",
            item.isRunning ? "RUNNING" : "STOPPED");
        info += "  当前 MAC: "
            + (item.macAddress.empty() ? std::string("-") : item.macAddress) + "\n";
        info += "  永久 MAC: "
            + (item.permanentMacAddress.empty()
                ? std::string("-") : item.permanentMacAddress) + "\n";
        info += StringUtils::format("  MTU: %u\n", item.mtu);

        for (size_t i = 0; i < item.ipv4Addresses.size(); ++i) {
            std::string suffix;
            if (i < item.ipv4Netmasks.size()) {
                uint8_t prefix = 0;
                if (NetworkUtils::netmaskToPrefixLength(item.ipv4Netmasks[i], prefix)) {
                    suffix = "/" + std::to_string(prefix);
                } else if (!item.ipv4Netmasks[i].empty()) {
                    suffix = "  掩码 " + item.ipv4Netmasks[i];
                }
            }
            info += "  IPv4: " + item.ipv4Addresses[i] + suffix + "\n";
        }

        for (size_t i = 0; i < item.ipv6Addresses.size(); ++i) {
            std::string suffix;
            if (i < item.ipv6Netmasks.size()) {
                uint8_t prefix = 0;
                if (NetworkUtils::netmaskToPrefixLength(item.ipv6Netmasks[i], prefix)) {
                    suffix = "/" + std::to_string(prefix);
                }
            }
            info += "  IPv6: " + item.ipv6Addresses[i] + suffix + "\n";
        }

        if (item.ipv4Addresses.empty() && item.ipv6Addresses.empty()) {
            info += "  IP: -\n";
        }
        info += "\n";
    }

    std::string gateway;
    std::string gatewayInterface;
    if (NetworkUtils::getDefaultGateway(gateway, &gatewayInterface)) {
        info += "默认网关: " + gateway + " (" + gatewayInterface + ")\n";
    } else {
        info += "默认网关: -\n";
    }

    const std::vector<std::string> dnsServers = NetworkUtils::getDnsServers();
    info += "DNS: ";
    if (dnsServers.empty()) {
        info += "-";
    } else {
        for (size_t i = 0; i < dnsServers.size(); ++i) {
            if (i != 0) info += ", ";
            info += dnsServers[i];
        }
    }
    info += "\n";

    interFace->setText(info.c_str());
}

FACTORY_ITEM_REGISTER(AppRid::to_network, FactoryNetwork);
