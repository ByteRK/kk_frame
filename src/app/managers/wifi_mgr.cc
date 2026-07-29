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
#include "json_utils.h"
#include <cdlog.h>
#include <core/app.h>
#include <core/systemclock.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

static const int WIFI_DATA_VERSION = 1;

WifiMgr::WifiMgr() :
    AutoSaveItem(2000, 10000) { }

WifiMgr::~WifiMgr() {
    stop();
}

void WifiMgr::init() {
    if (mInitialized) return;
    load();
    AutoSaveItem::init();

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
    const bool enabled = getSwitch();
    setSwitch(enabled);
    if (enabled) mAutoConnect = true;

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
    {
        std::lock_guard<std::mutex> lk(mApsMutex);
        mAps.clear();
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
    if (mSavedSsid != ssid || mSavedPsk != psk) {
        mSavedSsid = ssid;
        mSavedPsk = psk;
        mHaveChange = true;
    }
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
            if (mSwitch) {
                mSwitch = false;
                mHaveChange = true;
            }
            return;
        }
        if (!wasEnabled) mAutoConnect = true;
        scan();
    } else {
        mWifiHal->disable();
        mAutoConnect = false;
    }

    if (mSwitch != enable) {
        mSwitch = enable;
        mHaveChange = true;
    }
}

bool WifiMgr::getSwitch() {
    return mSwitch;
}

void WifiMgr::getConnectInfo(std::string& ssid, std::string& psk) {
    ssid = mSavedSsid;
    psk = mSavedPsk;
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

    if (mStateChanged.exchange(false)) { // 状态改变
        const WifiHal::State state = getState();
        if (state == WifiHal::State::Connected || state == WifiHal::State::IpReady) {
            updateResultAfterConnected();
        } else {
            clearConnectedResult();
        }
        std::vector<WiFiListener*> listeners;
        {
            std::lock_guard<std::mutex> lk(mListenersMutex);
            listeners.assign(mListeners.begin(), mListeners.end());
        }
        for (auto* cb : listeners)
            cb->onStateChanged();
    }

    if (mApsChanged.exchange(false)) { // 扫描结果改变
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
    Json::Value wifiJson;
    wifiJson["version"] = WIFI_DATA_VERSION;
    wifiJson["switch"] = mSwitch;
    wifiJson["credentials"]["ssid"] = mSavedSsid;
    wifiJson["credentials"]["psk"] = mSavedPsk;

    const bool saved = JsonUtils::save(
        isBackup ? WIFI_FILE_BAK_PATH : WIFI_FILE_PATH,
        wifiJson);
    if (saved && !isBackup) mHaveChange = false;
    return saved;
}

bool WifiMgr::haveChange() {
    return mHaveChange;
}

void WifiMgr::load() {
    mSwitch = WIFI_SWITCH;
    mSavedSsid = WIFI_SSID;
    mSavedPsk = WIFI_PASSWORD;
    mHaveChange = false;

    Json::Value wifiJson;
    std::string loadingPath = "";

    bool res = FileUtils::check(
        { WIFI_FILE_PATH, WIFI_FILE_BAK_PATH },
        [&wifiJson, &loadingPath](const std::string& file, size_t size) {
        if (size <= 0) return false;

        Json::Value candidate;
        if (!JsonUtils::load(file, candidate) || !candidate.isObject()) {
            return false;
        }
        if (JsonUtils::get<int>(candidate, "version", 0) != WIFI_DATA_VERSION) {
            LOGE("[wifi] unsupported data version. file=%s", file.c_str());
            return false;
        }

        wifiJson = candidate;
        loadingPath = file;
        return true;
    });

    if (!res) {
        LOG(ERROR) << "[wifi] no local data file found. use default data";
        mHaveChange = true;
        return;
    }

    const Json::Value credentials = wifiJson["credentials"];
    mSwitch = JsonUtils::get<bool>(wifiJson, "switch", WIFI_SWITCH);
    if (credentials.isObject()) {
        mSavedSsid =
            JsonUtils::get<std::string>(credentials, "ssid", WIFI_SSID);
        mSavedPsk =
            JsonUtils::get<std::string>(credentials, "psk", WIFI_PASSWORD);
    }
    LOG(INFO) << "[wifi] load local data. file=" << loadingPath;
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
    bool changed = false;
    for (auto& ap : mAps) {
        const bool connected = ap.ssid == ssid;
        if (ap.connected != connected) {
            ap.connected = connected;
            changed = true;
        }
    }
    if (changed) {
        mApsChanged.store(true); // 通知扫描结果改变
    }
}

void WifiMgr::clearConnectedResult() {
    bool changed = false;
    std::lock_guard<std::mutex> lk(mApsMutex);
    for (auto& ap : mAps) {
        if (ap.connected) {
            ap.connected = false;
            changed = true;
        }
    }
    if (changed) {
        mApsChanged.store(true);
    }
}
