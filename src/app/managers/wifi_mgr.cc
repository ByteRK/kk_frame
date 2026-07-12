/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 18:59:51
 * @LastEditTime: 2026-07-02 13:43:38
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
#include <cerrno>
#include <cstdio>
#include <cstring>

#define WIFI_SECTION "wifi_options"

WifiMgr::WifiMgr() :
    AutoSaveItem(2000, 10000) { }

WifiMgr::~WifiMgr() {
    stop();
}

void WifiMgr::init() {
    if (mInitialized) return;
    load();

    WifiHal::Options opt;
    opt.ifname = "wlan0";
    opt.dhcp_cmd = "udhcpc -i wlan0 -n -q";
    opt.auto_reconnect = true;
    mWifiHal.reset(new WifiHal(opt));
    mWifiHal->setCallbacks({
        [this](WifiHal::State s, const std::string& r) {onStateChanged(s,r);},
        [this](const std::vector<WifiHal::ApInfo>& aps) {onScanResult(aps);}
        });

    mInitialized = true;

    // WIFI状态同步
    setSwitch(getSwitch());
    if (getSwitch())mAutoConnect = true;

    // 延迟三秒
    cdroid::App::getInstance().addEventHandler(this);
    mEventHandlerRegistered = true;
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + 3000;
}

void WifiMgr::stop() {
    if (mEventHandlerRegistered) {
        cdroid::App::getInstance().removeEventHandler(this);
        mEventHandlerRegistered = false;
    }

    if (mWifiHal) {
        mWifiHal->setCallbacks({});
        mWifiHal->disable();
        mWifiHal.reset();
    }

    {
        std::lock_guard<std::mutex> lk(mStateMutex);
        mState = WifiHal::State::Off;
    }
    mStateChanged.store(false);
    mApsChanged.store(false);
    mAutoConnect = false;
    mInitialized = false;
}

void WifiMgr::reset() {
    const char* paths[] = { WIFI_FILE_PATH, WIFI_FILE_BAK_PATH };
    for (const char* path : paths) {
        if (std::remove(path) != 0 && errno != ENOENT) {
            LOGE("[wifi] remove option failed. file=%s errno=%d(%s)",
                path, errno, std::strerror(errno));
        }
    }
    FileUtils::sync();
    load();
    LOGE("[wifi] reset option.");
}

void WifiMgr::addListener(WiFiListener* listener) {
    if (!listener) return;
    std::lock_guard<std::mutex> lk(mListenersMutex);
    mListeners.insert(listener);
}

void WifiMgr::removeListener(WiFiListener* listener) {
    std::lock_guard<std::mutex> lk(mListenersMutex);
    mListeners.erase(listener);
}

bool WifiMgr::isConnected() {
    return getState() == WifiHal::State::IpReady;
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
    if (!mWifiHal || !mWifiHal->connect(ssid, psk)) return false;
    mOption.setValue(WIFI_SECTION, "SSID", ssid);
    mOption.setValue(WIFI_SECTION, "PSK", psk);
    return true;
}

bool WifiMgr::disconnect() {
    return mWifiHal ? mWifiHal->disconnect() : false;
}

void WifiMgr::setSwitch(bool enable) {
    if (!mWifiHal) return;

    const bool wasEnabled = mWifiHal->state() != WifiHal::State::Off;
    LOGI("[wifi] set switch. switch=%d", enable);
    if (enable) {
        if (!mWifiHal->enable()) {
            LOGE("[wifi] set switch failed. switch=1");
            if (getSwitch()) mOption.setValue(WIFI_SECTION, "SWITCH", false);
            return;
        }
        if (!wasEnabled) mAutoConnect = true;
        scan();
    } else {
        mWifiHal->disable();
        mAutoConnect = false;
    }

    if (enable != getSwitch()) {
        mOption.setValue(WIFI_SECTION, "SWITCH", enable);
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
    std::lock_guard<std::mutex> lk(mApsMutex);
    for (auto& i_ap : mAps) {
        if (i_ap.connected) {
            ap = i_ap;
            return true;
        }
    }
    return false;
}

int WifiMgr::checkEvents() {
    return cdroid::SystemClock::uptimeMillis() >= mNextEventTime;
}

int WifiMgr::handleEvents() {
    mNextEventTime = cdroid::SystemClock::uptimeMillis() + 1000;

    if (mAutoConnect) { // 自动连接
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
        if (isConnected())
            updateResultAfterConnected();
        std::vector<WiFiListener*> listeners;
        {
            std::lock_guard<std::mutex> lk(mListenersMutex);
            listeners.assign(mListeners.begin(), mListeners.end());
        }
        for (auto* cb : listeners)
            cb->onStateChanged();
    }

    if (mApsChanged.load()) { // 扫描结果改变
        mApsChanged.store(false);
        std::vector<WiFiListener*> listeners;
        {
            std::lock_guard<std::mutex> lk(mListenersMutex);
            listeners.assign(mListeners.begin(), mListeners.end());
        }
        for (auto* cb : listeners)
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

    bool res = FileUtils::check(
        { WIFI_FILE_PATH, WIFI_FILE_BAK_PATH },
        [this, &loadingPath](const std::string& file, size_t size) {
        if (size <= 0)return false;
        mOption.load(file);
        loadingPath = file;
        return true;
    });

    if (res) {
        LOG(INFO) << "[wifi] load option. file=" << loadingPath;
    } else {
        LOG(ERROR) << "[wifi] no option file found.";
    }
}

void WifiMgr::onStateChanged(WifiHal::State state, const std::string& reason) {
    {
        std::lock_guard<std::mutex> lk(mStateMutex);
        mState = state;
    }
    mStateChanged.store(true);
}

void WifiMgr::onScanResult(const std::vector<WifiHal::ApInfo>& aps) {
    {
        std::lock_guard<std::mutex> lk(mApsMutex);
        mAps = aps;
    }
    mApsChanged.store(true);
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
