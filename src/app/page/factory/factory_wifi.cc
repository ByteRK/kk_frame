/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_wifi.cc
 * @Description: WIFI检测
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_wifi.h"

#include "string_utils.h"

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
