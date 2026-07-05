/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-03 17:05:27
 * @LastEditTime: 2026-07-05 02:24:24
 * @FilePath: /kk_frame/src/app/page/view/page_factory_item.cc
 * @Description: 产测页面子项
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory_item.h"

#include "wifi_mgr.h"
#include "auth_mgr.h"
#include "config_mgr.h"

#include "touch_test_view.h"

#include "string_utils.h"
#include "arg_utils.h"
#include "network_utils.h"

#include "app_version.h"
#include "series_info.h"
#include <core/build.h>

/************************** 项目信息 **************************/

FactoryInfo::FactoryInfo(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setInfo();
}

void FactoryInfo::setInfo() {
    TextView* tv = PBase::get<TextView>(mRootView, AppRid::info);
    std::string info;

    // 软件信息
    info += "软件名称: " APP_NAME_STR "\n";
    info += "软件版本: V" APP_VERSION_D "\n";
    info += StringUtils::format("框架版本: Cdroid_V%s_%s\n", cdroid::Build::VERSION::Release, cdroid::Build::VERSION::CommitID);
    info += "代码哈希: " GIT_VERSION "\n";
    info += StringUtils::format("打包人员: %s\n", getlogin());
    info += "打包时间: " BUILD_DATE "\n";
    info += "打包详情: " APP_VER_INFO "\n";
    info += "运行命令: " + ArgUtils::getRawString() + "\n";

    info += "\n";

    // 硬件信息
    info += "核心: " CPU_BRAND " " CPU_NAME "\n";
    info += "内存: " MEMORY_NAME " " MEMORY_SIZE "\n";
    info += "闪存: " FLASH_NAME " " FLASH_SIZE "\n";
    info += "屏幕: " SCREEN_SIZE "\n";

    tv->setText(info.c_str());
}

/************************** 网络信息 **************************/

FactoryNetwork::FactoryNetwork(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
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

/************************** 授权信息 **************************/

FactoryAuth::FactoryAuth(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);

    mAuthInfo = PBase::get<TextView>(mRootView, AppRid::auth_info);
    mAuthRequest = PBase::get<TextView>(mRootView, AppRid::auth_request);
    mAuthRequestInfo = PBase::get<TextView>(mRootView, AppRid::auth_request_info);

    PBase::click(mAuthRequest, [](View&) {
        g_auth->request();
    });

    g_msg->add(MSG_AUTH_CODE, this);
}

FactoryAuth::~FactoryAuth() {
    g_msg->removeAll(this);
}

void FactoryAuth::onShow() {
    onMessage(MSG_AUTH_CODE, 0, nullptr);
}

void FactoryAuth::onMessage(int msgType, int msgValue, void* msgPtr) {
    if (msgType != MSG_AUTH_CODE) return;

    mAuthRequestInfo->setText(
        g_auth->mac() + "\n" + g_auth->authURL()
    );

    if (g_config->isAuthSuccess()) {
        mAuthInfo->setText(
            StringUtils::format(
                "授权码信息\nPIP: %s | DID: %s",
                g_config->getProductId().c_str(),
                g_config->getDeviceId().c_str()
            )
        );
        mAuthRequest->setText("再次请求");
        mAuthRequest->setActivated(true);
    } else {
        mAuthInfo->setText("当前不存在授权码");
        mAuthRequest->setText("申请授权码");
        mAuthRequest->setActivated(false);
    }
}

/************************** 功能开关 **************************/

FactorySwitch::FactorySwitch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setSwitch();
}

void FactorySwitch::setSwitch() { }

FactoryTouch::FactoryTouch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setTouch();
}

/************************** 触摸测试 **************************/

void FactoryTouch::onShow() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->resetTest();
}

void FactoryTouch::setTouch() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->setOnAllBlocksActivated([this]() {
        exit();
    });
}

/************************** 显示测试 **************************/

FactoryColor::FactoryColor(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setColor();
}

void FactoryColor::onShow() {
    cdroid::Drawable* bg = mRootView->getBackground();
    bg->setLevel(1);
}

void FactoryColor::setColor() {
    PBase::click(mRootView, [this](View& v) {
        cdroid::Drawable* bg = v.getBackground();
        int level = bg->getLevel();
        if (level < 7) {
            bg->setLevel(++level);
        } else {
            exit();
        }
    });
}

/************************** WIFI检测 **************************/

FactoryWifi::FactoryWifi(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);

    mWifiList = PBase::get<TextView>(mRootView, AppRid::wifi_list);
    mWifiConnInfo = PBase::get<TextView>(mRootView, AppRid::wifi_conn_info);
    mWifiRefresh = PBase::get<TextView>(mRootView, AppRid::refresh);
    mWifiSwitch = PBase::get<TextView>(mRootView, AppRid::wifi_switch);

    PBase::click(mWifiSwitch, [this](View&v) {
        g_wifi->setSwitch(!v.isActivated());
        updateWifiInfo();
    });
    PBase::click(mWifiRefresh, [this](View&) {g_wifi->scan();});
    PBase::click(mWifiConnInfo, [this](View&) {
#ifdef PRODUCT_X64
        g_wifi->connect("IMKK", "Abcd+1234");
#endif
    });

    g_wifi->addListener(this);
}

FactoryWifi::~FactoryWifi() {
    g_wifi->removeListener(this);
}

void FactoryWifi::onShow() {
    updateWifiInfo();
}

void FactoryWifi::onStateChanged() {
    updateWifiInfo();
}

void FactoryWifi::onScanResult() {
    updateWifiInfo();
}

void FactoryWifi::updateWifiInfo() {
    const bool open = g_wifi->getSwitch();
    mWifiSwitch->setActivated(open);
    mWifiSwitch->setText(open ? "WIFI: ON" : "WIFI: OFF");
    mWifiRefresh->setVisibility(open ? View::VISIBLE : View::GONE);
    mWifiConnInfo->setVisibility(open ? View::VISIBLE : View::GONE);
    mWifiList->setVisibility(open ? View::VISIBLE : View::GONE);

    if (!open) {
        mWifiConnInfo->setText("未连接");
        mWifiConnInfo->setActivated(false);
        mWifiList->setText("");
        return;
    }

    const WifiHal::State state = g_wifi->getState();
    const char* stateText = "空闲";
    switch (state) {
    case WifiHal::State::Off:          stateText = "已关闭"; break;
    case WifiHal::State::Idle:         stateText = "空闲"; break;
    case WifiHal::State::Scanning:     stateText = "扫描中"; break;
    case WifiHal::State::Connecting:   stateText = "连接中"; break;
    case WifiHal::State::Connected:    stateText = "获取 IP 中"; break;
    case WifiHal::State::IpReady:      stateText = "已连接"; break;
    case WifiHal::State::Disconnected: stateText = "连接已断开"; break;
    case WifiHal::State::AuthFailed:   stateText = "认证失败"; break;
    case WifiHal::State::ApNotFound:   stateText = "未找到热点"; break;
    }

    std::vector<WifiHal::ApInfo> aps;
    g_wifi->getAps(aps);

    std::string connection = stateText;
#if 0
    if (state == WifiHal::State::IpReady) {
        for (const auto& ap : aps) {
            if (ap.connected) {
                connection += ": " + ap.ssid;
                break;
            }
        }
    }
#endif
    mWifiConnInfo->setText(connection.c_str());
    mWifiConnInfo->setActivated(state == WifiHal::State::IpReady);

    std::string list;
    if (state == WifiHal::State::Scanning && aps.empty()) {
        list = "正在扫描热点...";
    } else if (aps.empty()) {
        list = "未扫描到热点";
    } else {
        constexpr size_t STATE_WIDTH = 12;
        constexpr size_t SECURITY_WIDTH = 8;
        constexpr size_t SIGNAL_WIDTH = 15;
        constexpr size_t FREQUENCY_WIDTH = 14;
        auto column = [](const std::string& text, size_t width) {
            std::string value = StringUtils::substringByChars(text.c_str(), width, 2);
            const size_t valueWidth = StringUtils::characterCount(value.c_str(), 2);
            value.append(width - valueWidth, ' ');
            return value;
        };

        list = column("状态", STATE_WIDTH)
            + column("安全", SECURITY_WIDTH)
            + column("信号", SIGNAL_WIDTH)
            + column("频率", FREQUENCY_WIDTH)
            + "SSID\n";
        for (const auto& ap : aps) {
            const std::string ssid = ap.ssid.empty() ? "<隐藏网络>" : ap.ssid;
            list += column(ap.connected ? "已连接" : "-", STATE_WIDTH)
                + column(ap.encrypted ? "加密" : "开放x", SECURITY_WIDTH)
                + column(std::to_string(ap.signal) + " dBm", SIGNAL_WIDTH)
                + column(std::to_string(ap.freq) + " MHz", FREQUENCY_WIDTH)
                + ssid + "\n";
        }
    }
    mWifiList->setText(list.c_str());
}

/************************** 屏幕老化 **************************/

FactoryAging::FactoryAging(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    PBase::click(mRootView, [this](View&) {exit();});
    mTicker.setTick(3000);
    mTicker.setCallBack(std::bind(&FactoryAging::onTick, this, std::placeholders::_1));
}

void FactoryAging::onShow() {
    mRootView->getBackground()->setLevel(1);
    mTicker.startTick();
}

void FactoryAging::onHide() {
    mTicker.stopTick();
}

void FactoryAging::onTick(int64_t) {
    cdroid::Drawable* bg = mRootView->getBackground();
    int level = bg->getLevel();
    if (level < 5) {
        bg->setLevel(++level);
    } else {
        bg->setLevel(1);
    }
}
