/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:43:34
 * @LastEditTime: 2026-02-28 18:26:31
 * @FilePath: /kk_frame/src/comm/wifi/wifi_hal.cc
 * @Description: WiFi 管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wifi_hal.h"
#include "encoding_utils.h"
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cdlog.h>
#include <core/systemclock.h>

static bool GetIpv4(const std::string& ifname, std::string& outIp) {
    outIp.clear();
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname.c_str());

    if (::ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        auto* addr = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr);
        outIp = ::inet_ntoa(addr->sin_addr);
        ::close(fd);
        return !outIp.empty() && outIp != "0.0.0.0";
    }

    ::close(fd);
    return false;
}

static inline bool starts_with(const std::string& s, const char* prefix) {
    return s.compare(0, ::strlen(prefix), prefix) == 0;
}

static WpaClient::Options MakeWpaOpt(const WifiHal::Options& opt) {
    WpaClient::Options w;
    w.ctrl_path = opt.ctrl_path;
    w.ifname = opt.ifname;
    w.cmd_timeout_ms = 2000;
    return w;
}

WifiHal::WifiHal(const Options& opt)
    : mOpt(opt), mWpa(MakeWpaOpt(opt)) {
}

WifiHal::~WifiHal() {
    disable();
}

void WifiHal::setCallbacks(const Callbacks& cb) {
    std::lock_guard<std::mutex> lk(mMtx);
    mCb = cb;
}

bool WifiHal::enable() {
    if (state() != State::Off) return true;

    std::system(mOpt.ifup_cmd.c_str());

    if (!mWpa.open()) {
        setState(State::Off, "wpa_ctrl_open failed (is wpa_supplicant running?)");
        return false;
    }

    bool ok = mWpa.startMonitor([this](const std::string& msg) { this->onWpaEvent(msg); });
    if (!ok) {
        setState(State::Off, "wpa_ctrl_attach failed");
        return false;
    }

    mUserDisconnect.store(false);
    setState(State::Idle, "wifi enabled");
    return true;
}

void WifiHal::disable() {
    cancelReconnect();

    if (state() == State::Off) return;
    setState(State::Off, "wifi disabled");

    mWpa.stopMonitor();
    mWpa.close();

    std::system(mOpt.ifdown_cmd.c_str());
}

bool WifiHal::scan() {
    if (state() == State::Off) return false;   // state() 自己会加锁
    // setState(State::Scanning, "scan requested");

    std::string reply;
    if (!runCmd("SCAN", &reply)) {
        setState(State::Idle, "SCAN cmd failed");
        return false;
    }
    return true;
}

bool WifiHal::connect(const std::string& ssid, const std::string& psk) {
    if (state() == State::Off) return false;

    {
        std::lock_guard<std::mutex> lk(mMtx);
        mUserDisconnect.store(false);
        mLastSsid = ssid;
        mLastPsk = psk;
    }
    setState(State::Connecting, "connect requested");

    cancelReconnect();

    if (!ensureNetworkConfigured(ssid, psk)) {
        setState(State::Disconnected, "configure network failed");
        return false;
    }

    std::string reply;
    if (!runCmd("RECONNECT", &reply)) {
        setState(State::Disconnected, "RECONNECT cmd failed");
        return false;
    }
    return true;
}

bool WifiHal::disconnect() {
    mUserDisconnect.store(true);
    cancelReconnect();

    std::string reply;
    bool ok = runCmd("DISCONNECT", &reply);
    setState(State::Disconnected, ok ? "user disconnect" : "DISCONNECT cmd failed");
    return ok;
}

WifiHal::State WifiHal::state() const {
    std::lock_guard<std::mutex> lk(mMtx);
    return mState;
}

bool WifiHal::getStatus(std::string& outStatusText) {
    std::string reply;
    if (!runCmd("STATUS", &reply)) return false;
    outStatusText = reply;
    return true;
}

WifiHal::Callbacks WifiHal::getCallbacks() {
    std::lock_guard<std::mutex> lk(mMtx);
    return mCb;
}

void WifiHal::onWpaEvent(const std::string& msg) {
    LOGV("onWpaEvent: %s", msg.c_str());

    // 常见事件：
    // CTRL-EVENT-SCAN-RESULTS
    // CTRL-EVENT-CONNECTED
    // CTRL-EVENT-DISCONNECTED
    // CTRL-EVENT-SSID-TEMP-DISABLED（密码错/握手失败常见）
    // CTRL-EVENT-ASSOC-REJECT（AP 不接受）
    if (starts_with(msg, WPA_EVENT_SCAN_RESULTS)) {
        std::vector<ApInfo> aps;
        if (parseScanResults(aps)) {
            // 信号从大到小
            std::sort(aps.begin(), aps.end(), [](const ApInfo& a, const ApInfo& b) {
                return a.signal > b.signal;
            });

            Callbacks cb = getCallbacks();
            // if (state() == State::Scanning) setState(State::Idle, "scan done");
            if (cb.onScanDone) cb.onScanDone(aps);
        } else {
            Callbacks cb = getCallbacks();
            setState(State::Idle, "scan done but parse failed");
            if (cb.onScanDone) cb.onScanDone(std::vector<ApInfo>{});
        }
        return;
    }

    if (starts_with(msg, WPA_EVENT_CONNECTED)) {
        setState(State::Connected, "CTRL-EVENT-CONNECTED");

        // 连接已经起来：清理重连抖动控制
        mReconnFailCount.store(0);
        mReconnInFlight.store(false);

        std::thread(&WifiHal::startDhcp, this).detach();
        return;
    }

    if (starts_with(msg, WPA_EVENT_DISCONNECTED)) {
        setState(State::Disconnected, "CTRL-EVENT-DISCONNECTED");

        // 允许后续重连再次发起，但避免短时间多次 kick
        mReconnInFlight.store(false);

        // 非人为断开 -> 自动重连
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("disconnected");
        }
        return;
    }

    if (starts_with(msg, WPA_EVENT_TEMP_DISABLED)) {
        // 常见：4WAY_HANDSHAKE / auth fail
        setState(State::AuthFailed, "SSID TEMP DISABLED (auth failed?)");
        mReconnInFlight.store(false);
        // 密码错误场景一般不建议自动重连（避免狂刷）
        return;
    }

    if (starts_with(msg, WPA_EVENT_ASSOC_REJECT)) {
        setState(State::ApNotFound, "ASSOC REJECT");
        mReconnInFlight.store(false);
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("assoc reject");
        }
        return;
    }

    // 其他事件：在这里扩展更多解析/日志
}

bool WifiHal::reconnectLight() {
    int netId = -1;
    {
        std::lock_guard<std::mutex> lk(mMtx);
        netId = mLastNetId;
    }
    if (netId < 0) return false;

    // 轻重连：不做 DISCONNECT / REMOVE_NETWORK
    runCmd("ENABLE_NETWORK " + std::to_string(netId), nullptr);
    runCmd("SELECT_NETWORK " + std::to_string(netId), nullptr);
    return runCmd("RECONNECT", nullptr);
}

bool WifiHal::maybeScanForReconnect() {
    uint64_t now = cdroid::SystemClock::uptimeMillis();
    uint64_t last = mLastScanMs.load();
    if (now - last < mOpt.scan_min_interval_ms) return false;
    mLastScanMs.store(now);
    std::string reply;
    return runCmd("SCAN", &reply);
}

void WifiHal::setState(State s, const std::string& reason) {
    Callbacks cb;
    {
        std::lock_guard<std::mutex> lk(mMtx);
        if (mState == s) return;
        mState = s;
        cb = mCb;
    }
    if (cb.onStateChanged) cb.onStateChanged(s, reason);
}

bool WifiHal::runCmd(const std::string& cmd, std::string* reply) {
    std::string r;
    bool ok = mWpa.request(cmd, r);
    if (!ok) return false;
    if (reply) *reply = r;

    // wpa_cli 风格：成功一般返回 "OK" 或输出内容；失败返回 "FAIL"
    if (r == "FAIL") return false;
    return true;
}

bool WifiHal::ensureNetworkConfigured(const std::string& ssid, const std::string& psk) {
    // 简化策略：每次连接都 “REMOVE_NETWORK all -> ADD_NETWORK -> SET_NETWORK -> ENABLE -> SELECT -> SAVE_CONFIG”
    // 优点：逻辑稳定、不依赖历史残留 network id
    std::string reply;

    // 先断开，避免状态粘连
    runCmd("DISCONNECT", nullptr);

    // 清空旧网络
    runCmd("REMOVE_NETWORK all", nullptr);

    if (!runCmd("ADD_NETWORK", &reply)) return false;
    // reply 是 network id
    const std::string netId = reply;

    {
        std::lock_guard<std::mutex> lk(mMtx);
        mLastNetId = std::atoi(netId.c_str());
    }

    // ssid/psk 需要带引号
    if (!runCmd("SET_NETWORK " + netId + " ssid " + "\"" + ssid + "\"", nullptr)) return false;

    if (psk.empty()) {
        if (!runCmd("SET_NETWORK " + netId + " key_mgmt NONE", nullptr)) return false;
    } else {
        if (!runCmd("SET_NETWORK " + netId + " psk " + "\"" + psk + "\"", nullptr)) return false;
    }

    // 启用 scan_ssid=1 扫描隐藏 SSID
    runCmd("SET_NETWORK " + netId + " scan_ssid 1", nullptr);

    // 允许同 SSID 多 BSSID 漫游：交给 wpa_supplicant 做 BSS 选择；bgscan 帮助后台扫描
    // 越大越省电/越少扫描
    runCmd("SET_NETWORK " + netId + " bgscan \"simple:120:-67:900\"", nullptr);

    if (!runCmd("ENABLE_NETWORK " + netId, nullptr)) return false;
    if (!runCmd("SELECT_NETWORK " + netId, nullptr)) return false;

    // wpa_supplicant.conf 开启了 update_config=1，SAVE_CONFIG 才有效
    runCmd("SAVE_CONFIG", nullptr);
    return true;
}

bool WifiHal::startDhcp() {
    // 运行 DHCP
    int ret = std::system(mOpt.dhcp_cmd.c_str());

    // 等待IP出现（最多5秒）
    std::string ip;
    bool ok = false;
    for (int i = 0; i < 50; ++i) { // 50 * 100ms = 5s
        if (GetIpv4(mOpt.ifname, ip)) { ok = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (ok) {
        setState(State::IpReady, std::string("dhcp ok ip=") + ip);
        mReconnFailCount.store(0);
        mReconnInFlight.store(false);
    } else {
        // DHCP 失败
        setState(State::Disconnected, "dhcp failed (no ip)");
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("dhcp failed");
        }
    }
    return true;
}

bool WifiHal::parseScanResults(std::vector<ApInfo>& out) {
    out.clear();

    // SCAN_RESULTS 输出类似：
    // bssid / frequency / signal level / flags / ssid
    // fa:be:81:c3:dc:30       2462    -40     [WPA2-PSK-CCMP][WPS][ESS][P2P]  Ricken
    std::string reply;
    if (!runCmd("SCAN_RESULTS", &reply)) return false;

    std::istringstream iss(reply);
    std::string line;
    LOGV("SCAN_RESULTS RAW:\n %s", reply.c_str());

    std::string connected_ssid("");
    if (state() == State::IpReady) {
        std::lock_guard<std::mutex> lk(mMtx);
        connected_ssid = mLastSsid;
    }

    bool first = true;
    while (std::getline(iss, line)) {
        if (first) { // header
            first = false;
            continue;
        }
        if (line.empty()) continue;

        std::vector<std::string> cols;
        {
            std::string tmp;
            std::istringstream ls(line);
            while (std::getline(ls, tmp, '\t')) cols.push_back(tmp);
        }
        if (cols.size() < 5) continue;

        ApInfo ap;
        ap.bssid = EncodingUtils::hexEscapes(cols[0]);
        ap.freq = std::atoi(cols[1].c_str());
        ap.signal = std::atoi(cols[2].c_str());
        ap.encrypted = (cols[3].find("WPA") != std::string::npos) || (cols[3].find("WEP") != std::string::npos);
        ap.ssid = cols[4];

        // 过滤空 SSID
        if (!ap.ssid.empty()) {
            ap.connected = ap.ssid == connected_ssid;

            bool ssid_exists = false; // 过滤重复的 SSID
            for (ApInfo& x : out) {
                if (x.ssid == ap.ssid) {
                    if (x.signal < ap.signal) x = ap; // 保留信号好的
                    ssid_exists = true;
                    break;
                }
            }
            if (!ssid_exists) out.push_back(ap);
        }
    }

    return true;
}

void WifiHal::scheduleReconnect(const std::string& reason) {
    // 如果当前已经有一次自动重连在飞，避免反复 kick
    if (mReconnInFlight.load()) return;

    {
        std::lock_guard<std::mutex> lk(mMtx);
        if (mLastSsid.empty()) return;
    }

    {
        std::lock_guard<std::mutex> lk(mReconnMtx);
        if (!mReconnRunning.load()) {
            mReconnRunning.store(true);
            mReconnBackoffMs = std::max(mOpt.reconnect_initial_ms, 100);
            mReconnTh = std::thread(&WifiHal::reconnectThread, this);
        }
    }
    // reason 预留做日志/上报
    mReconnCv.notify_all();
}

void WifiHal::cancelReconnect() {
    if (!mReconnRunning.exchange(false)) return;
    mReconnCv.notify_all();
    if (mReconnTh.joinable()) mReconnTh.join();
}

void WifiHal::reconnectThread() {
    std::unique_lock<std::mutex> lk(mReconnMtx);

    while (mReconnRunning.load()) {
        // 等待触发，或定时重试
        mReconnCv.wait_for(lk, std::chrono::milliseconds(mReconnBackoffMs));

        if (!mReconnRunning.load()) break;
        if (mUserDisconnect.load()) continue;

        // 若已经连接则不重试
        if (state() == State::IpReady) continue;

        // 重连在飞则跳过本轮
        if (mReconnInFlight.exchange(true)) continue;

        setState(State::Connecting, "auto reconnect");

        bool ok = reconnectLight();
        if (!ok) {
            int fails = ++mReconnFailCount;
            if (fails >= mOpt.reconn_fail_before_scan) {
                if (maybeScanForReconnect())
                    mReconnFailCount.store(0);
            }
            mReconnInFlight.store(false);
        }

        // 指数退避
        mReconnBackoffMs = std::min(mReconnBackoffMs * 2, mOpt.reconnect_max_ms);
    }
}