/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 18:59:51
 * @LastEditTime: 2026-02-28 18:13:51
 * @FilePath: /kk_frame/src/app/managers/wifi_mgr.cc
 * @Description: WIFI 管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wifi_mgr.h"
#include "config_info.h"
#include "file_utils.h"
#include "global_data.h"
#include <cdlog.h>
#include <core/app.h>
#include <core/systemclock.h>

#define WIFI_SECTION "wifi_options"

WifiMgr::WifiMgr() :
    AutoSaveItem(2000, 10000) {
    mWifiHal = nullptr;
}

WifiMgr::~WifiMgr() {
    if (mWifiHal)delete mWifiHal;
}

void WifiMgr::init() {
    load();

    WifiHal::Options opt;
    opt.ifname = "wlan0";
    opt.dhcp_cmd = "udhcpc -i wlan0 -n -q";
    opt.auto_reconnect = true;
    mWifiHal = new WifiHal(opt);
    mWifiHal->setCallbacks({
        [this](WifiHal::State s, const std::string& r) {onStateChanged(s,r);},
        [this](const std::vector<WifiHal::ApInfo>& aps) {onScanResult(aps);}
        });

    // WIFI状态同步
    setSwitch(getSwitch());
    if (getSwitch())mAutoConnect = true;

    // 延迟三秒
    cdroid::App::getInstance().addEventHandler(this);
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + 3000;
}

void WifiMgr::reset() {
    std::string command = std::string("rm")\
        + " " + WIFI_FILE_PATH + " " + WIFI_FILE_BAK_PATH;
    std::system(command.c_str());
    FileUtils::sync();
    load();
    LOGE("[wifi] reset option.");
}

void WifiMgr::addListener(WiFiListener* listener) {
    mListeners.insert(listener);
}

void WifiMgr::removeListener(WiFiListener* listener) {
    mListeners.erase(listener);
}

WifiHal::State WifiMgr::getState() {
    std::lock_guard<std::mutex> lk(mStateMutex);
    return mState;
}

void WifiMgr::getAps(std::vector<WifiHal::ApInfo>& aps) {
    std::lock_guard<std::mutex> lk(mApsMutex);
    aps = mAps;
}

bool WifiMgr::scan() {
    return mWifiHal ? mWifiHal->scan() : false;
}

bool WifiMgr::connect(const std::string& ssid, const std::string& psk) {
    mOption.setValue(WIFI_SECTION, "SSID", ssid);
    mOption.setValue(WIFI_SECTION, "PSK", psk);
    return mWifiHal ? mWifiHal->connect(ssid, psk) : false;
}

bool WifiMgr::disconnect() {
    return mWifiHal ? mWifiHal->disconnect() : false;
}

void WifiMgr::setSwitch(bool enable) {
    if (enable != getSwitch()) {
        mOption.setValue(WIFI_SECTION, "SWITCH", enable);
    }
    if (mWifiHal) {
        bool nowEnable = (mWifiHal->state() != WifiHal::State::Off);
        LOGE("[wifi] set switch. switch=%d", enable);
        bool res = true;
        if (enable) {
            res = mWifiHal->enable();
            if (res) scan();
            if (!nowEnable) mAutoConnect = true;
        } else mWifiHal->disable();
        if (!res) {
            // mOption.setValue(WIFI_SECTION, "SWITCH", !enable);
            LOGE("[wifi] set switch failed. switch=%d", enable);
        }
    }
}

bool WifiMgr::getSwitch() {
    return mOption.getBool(WIFI_SECTION, "SWITCH", WIFI_SWITCH);
}

void WifiMgr::getConnectInfo(std::string& ssid, std::string& psk) {
    ssid = mOption.getString(WIFI_SECTION, "SSID", WIFI_SSID);
    psk = mOption.getString(WIFI_SECTION, "PSK", WIFI_PASSWORD);
}

bool WifiMgr::getConnectedAp(WifiHal::ApInfo& ap) {
    for (auto& ap : mAps) {
        if (ap.connected) {
            ap = ap;
            return true;
        }
    }
    return false;
}

int WifiMgr::checkEvents() {
    return cdroid::SystemClock::uptimeMillis() >= mNextEventTime;
}

int WifiMgr::handleEvents() {
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + 500;

    if (mAutoConnect) {
        mAutoConnect = false;
        WifiHal::State state = getState();
        LOGI("[wifi] auto connect. state=%d", state);
        switch (state) {
        case WifiHal::State::Off:
        case WifiHal::State::Connected:
        case WifiHal::State::IpReady:
            break;
        default: {
            std::string ssid, psk;
            getConnectInfo(ssid, psk);
            if (!ssid.empty()) connect(ssid, psk);
        }   break;
        }
    }

    if (mStateChanged.load()) { // 状态改变
        mStateChanged.store(false);
        if (getState() == WifiHal::State::IpReady)
            updateResultAfterConnected();
        for (auto& cb : mListeners)
            cb->onStateChanged();
    }

    if (mApsChanged.load()) { // 扫描结果改变
        mApsChanged.store(false);
        for (auto& cb : mListeners)
            cb->onScanResult();
    }

    return 0;
}

bool WifiMgr::save(bool isBackup) {
    mOption.save(
        isBackup ? WIFI_FILE_BAK_PATH : WIFI_FILE_PATH
    );
    return true;
}

bool WifiMgr::haveChange() {
    return mOption.getUpdates();
}

void WifiMgr::load() {
    mOption = cdroid::Preferences(); // 清空原配置

    std::string loadingPath = "";
    size_t fileLen = 0;
    if (FileUtils::check(WIFI_FILE_PATH, &fileLen) && fileLen > 0) {
        loadingPath = WIFI_FILE_PATH;
    } else if (FileUtils::check(WIFI_FILE_BAK_PATH, &fileLen) && fileLen > 0) {
        loadingPath = WIFI_FILE_BAK_PATH;
    } else {
        LOG(ERROR) << "[wifi] no option file found. use default data.";
        mOption.setValue(WIFI_SECTION, "HELLO", std::string("WORLD!!!"));
        return;
    }

    mOption.load(loadingPath);
    LOG(INFO) << "[wifi] load option. file=" << loadingPath;
}

void WifiMgr::onStateChanged(WifiHal::State state, const std::string& reason) {
    mStateChanged.store(true);
    std::lock_guard<std::mutex> lk(mStateMutex);
    mState = state;
}

void WifiMgr::onScanResult(const std::vector<WifiHal::ApInfo>& aps) {
    mApsChanged.store(true);
    std::lock_guard<std::mutex> lk(mApsMutex);
    mAps = aps;
}

void WifiMgr::updateResultAfterConnected() {
    std::string ssid, psk;
    getConnectInfo(ssid, psk);
    std::lock_guard<std::mutex> lk(mApsMutex);
    for (auto& ap : mAps) {
        if (ap.ssid == ssid)ap.connected = true;
        else ap.connected = false;
    }
    mApsChanged.store(true); // 通知扫描结果改变
}
