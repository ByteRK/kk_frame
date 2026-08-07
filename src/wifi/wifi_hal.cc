/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:43:34
 * @LastEditTime: 2026-07-29 11:53:10
 * @FilePath: /kk_frame/src/wifi/wifi_hal.cc
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
#include <random>
#include <cctype>
#include <cerrno>
#include <limits>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cdlog.h>
#include <core/systemclock.h>

#ifdef PRODUCT_X64
#include "timer_mgr.h"

class WifiHal::X64DelayTimer {
public:
    explicit X64DelayTimer(const std::function<void()>& callback)
        : mCallback(callback) { }

    ~X64DelayTimer() {
        cancel();
    }

    bool schedule(int64_t delayMs) {
        cancel();
        if (delayMs <= 0) return false;
        mTimer = g_timer->scopedAfter(delayMs,
            [this](TimerMgr::TimerId, uint32_t) {
                if (mCallback) mCallback();
            });
        return mTimer.isActive();
    }

    void cancel() {
        mTimer.cancel();
    }

private:
    std::function<void()> mCallback;
    TimerMgr::TimerHandle mTimer;
};
#endif // PRODUCT_X64

static const char kWpaEventScanFailed[]      = "CTRL-EVENT-SCAN-FAILED ";
static const char kWpaEventNetworkNotFound[] = "CTRL-EVENT-NETWORK-NOT-FOUND ";

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

static std::string HexEncode(const std::string& value) {
    static const char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char ch : value) {
        result.push_back(kHex[ch >> 4]);
        result.push_back(kHex[ch & 0x0f]);
    }
    return result;
}

static bool EncodeWpaQuoted(const std::string& value, std::string& result) {
    result.clear();
    result.reserve(value.size() + 2);
    result.push_back('"');
    for (unsigned char ch : value) {
        // Control characters can terminate or alter a wpa control command.
        if (ch < 0x20 || ch == 0x7f) return false;
        if (ch == '\\' || ch == '"') result.push_back('\\');
        result.push_back(static_cast<char>(ch));
    }
    result.push_back('"');
    return true;
}

static bool IsHexString(const std::string& value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

static bool ParseNonNegativeInt(const std::string& value, int& result) {
    if (value.empty()) return false;

    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' ||
        parsed < 0 || parsed > std::numeric_limits<int>::max()) {
        return false;
    }

    result = static_cast<int>(parsed);
    return true;
}

static bool GetWpaEventScanId(const std::string& msg, int& scanId) {
    std::istringstream iss(msg);
    std::string token;
    while (iss >> token) {
        static const char kIdPrefix[] = "id=";
        if (!starts_with(token, kIdPrefix)) continue;
        return ParseNonNegativeInt(token.substr(sizeof(kIdPrefix) - 1), scanId);
    }
    return false;
}

static std::string getWpaEventParam(const std::string& msg, const std::string& key) {
    const std::string needle = key + "=";
    auto pos = msg.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    auto end = msg.find_first_of(" \t\r\n", pos);
    return msg.substr(pos, end == std::string::npos ? end : end - pos);
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
#ifdef PRODUCT_X64
    mX64ScanResultTimer.reset(new X64DelayTimer([this]() {
        onX64ScanResultTimer();
    }));
    mX64ConnectResultTimer.reset(new X64DelayTimer([this]() {
        onX64ConnectResultTimer();
    }));
#endif
}

WifiHal::~WifiHal() {
    disable();
}

#ifdef PRODUCT_X64
void WifiHal::onX64ScanResultTimer() {
    const int scanId = mPendingScanId.load();
    if (state() != State::Off && scanId >= 0) {
        onWpaEvent("CTRL-EVENT-SCAN-RESULTS id=" + std::to_string(scanId));
    }
}

void WifiHal::onX64ConnectResultTimer() {
    if (state() == State::Connecting) {
        setState(State::IpReady, "dhcp ok ip=127.0.0.1");
    }
}
#endif

void WifiHal::setCallbacks(const Callbacks& cb) {
    std::lock_guard<std::mutex> lk(mMtx);
    mCb = cb;
}

bool WifiHal::enable() {
#if ENABLED(WIFI)
    if (state() != State::Off) return true;
    mShuttingDown.store(false);
#ifndef PRODUCT_X64
    std::system(mOpt.ifup_cmd.c_str());
#else
    LOGI("WifiHal::enable run system(%s)", mOpt.ifup_cmd.c_str());
#endif // PRODUCT_X64
    if (!mWpa.open()) {
        setState(State::Off, "wpa_ctrl_open failed (is wpa_supplicant running?)");
        return false;
    }
    bool ok = mWpa.startMonitor([this](const std::string& msg) { this->onWpaEvent(msg); });
    if (!ok) {
        mWpa.close();
        setState(State::Off, "wpa_ctrl_attach failed");
        return false;
    }
    mUserDisconnect.store(false);
    setState(State::Idle, "wifi enabled");
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

void WifiHal::disable() {
#if ENABLED(WIFI)
    mShuttingDown.store(true);
    mPendingScanId.store(-1);
    cancelReconnect();
#ifdef PRODUCT_X64
    if (mX64ScanResultTimer) mX64ScanResultTimer->cancel();
    if (mX64ConnectResultTimer) mX64ConnectResultTimer->cancel();
#endif
    const bool wasOff = state() == State::Off;
    if (!wasOff) setState(State::Off, "wifi disabled");
    mWpa.stopMonitor();
    stopDhcpWorker();
    mWpa.close();
#ifndef PRODUCT_X64
    if (!wasOff) std::system(mOpt.ifdown_cmd.c_str());
#else
    LOGI("WifiHal::disable run system(%s)", mOpt.ifdown_cmd.c_str());
#endif // PRODUCT_X64
#else
    LOGE("WIFI is not enabled");
#endif // ENABLED(WIFI)
}

bool WifiHal::scan() {
#if ENABLED(WIFI)
    if (state() == State::Off) return false;   // state() 自己会加锁
    if (mDriverReloading.load()) {
        LOGW("scan rejected: driver reload in progress");
        return false;
    }
    // setState(State::Scanning, "scan requested");
    std::string reply;
    if (!runCmd("SCAN use_id=1", &reply)) {
        LOGE("SCAN use_id=1 command failed");
        return false;
    }

    int scanId = -1;
#ifdef PRODUCT_X64
    scanId = 1;
#else
    if (!ParseNonNegativeInt(reply, scanId)) {
        LOGE("invalid SCAN use_id=1 reply: %s", reply.c_str());
        return false;
    }
#endif
    mPendingScanId.store(scanId);

#ifdef PRODUCT_X64
    if (!mX64ScanResultTimer ||
        !mX64ScanResultTimer->schedule(mOpt.x64_scan_delay_ms)) {
        mPendingScanId.store(-1);
        return false;
    }
#endif
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

bool WifiHal::connect(const std::string& ssid, const std::string& psk) {
#if ENABLED(WIFI)
    if (state() == State::Off) return false;
    if (mDriverReloading.load()) {
        LOGW("connect rejected: driver reload in progress");
        return false;
    }
    {
        std::lock_guard<std::mutex> lk(mMtx);
        mUserDisconnect.store(false);
        mLastSsid = ssid;
        mLastPsk = psk;
    }
    setState(State::Connecting, "connect requested");
    cancelReconnect();
#ifndef PRODUCT_X64
    if (!ensureNetworkConfigured(ssid, psk)) {
        setState(State::Disconnected, "configure network failed");
        return false;
    }
    // std::string reply;
    // if (!runCmd("RECONNECT", &reply)) {
    //     setState(State::Disconnected, "RECONNECT cmd failed");
    //     return false;
    // }
#else
    if (!mX64ConnectResultTimer ||
        !mX64ConnectResultTimer->schedule(mOpt.x64_connect_delay_ms)) {
        setState(State::Disconnected, "schedule connect result failed");
        return false;
    }
#endif
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

bool WifiHal::disconnect() {
#if ENABLED(WIFI)
    mUserDisconnect.store(true);
    cancelReconnect();
    stopDhcpWorker();
#ifdef PRODUCT_X64
    if (mX64ConnectResultTimer) mX64ConnectResultTimer->cancel();
#endif
    std::string reply;
    bool ok = runCmd("DISCONNECT", &reply);
    setState(State::Disconnected, ok ? "user disconnect" : "DISCONNECT cmd failed");
    return ok;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

WifiHal::State WifiHal::state() const {
#if ENABLED(WIFI)
    std::lock_guard<std::mutex> lk(mMtx);
    return mState;
#else
    LOGE("WIFI is not enabled");
    return State::Off;
#endif // ENABLED(WIFI)
}

bool WifiHal::getStatus(std::string& outStatusText) {
#if ENABLED(WIFI)
    std::string reply;
    if (!runCmd("STATUS", &reply)) return false;
    outStatusText = reply;
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

WifiHal::Callbacks WifiHal::getCallbacks() {
    std::lock_guard<std::mutex> lk(mMtx);
    return mCb;
}

void WifiHal::onWpaEvent(const std::string& msg) {
#if ENABLED(WIFI)
    LOGV("[WifiHal] onWpaEvent: %s", msg.c_str());
    if (mShuttingDown.load()) return;

    // 常见事件：
    // CTRL-EVENT-SCAN-RESULTS
    // CTRL-EVENT-CONNECTED
    // CTRL-EVENT-DISCONNECTED
    // CTRL-EVENT-SSID-TEMP-DISABLED（密码错/握手失败常见）
    // CTRL-EVENT-ASSOC-REJECT（AP 不接受）
    if (starts_with(msg, WPA_EVENT_SCAN_RESULTS)) {
        int eventScanId = -1;
        if (!GetWpaEventScanId(msg, eventScanId)) {
            LOGV("ignore scan results without active scan id (likely bgscan)");
            return;
        }

        int expectedScanId = eventScanId;
        if (!mPendingScanId.compare_exchange_strong(expectedScanId, -1)) {
            LOGV("ignore scan results id=%d, pending active scan id=%d",
                eventScanId, expectedScanId);
            return;
        }

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
            LOGE("active scan results parse failed. id=%d", eventScanId);
            if (cb.onScanDone) cb.onScanDone(std::vector<ApInfo>{});
        }
        return;
    }

    if (starts_with(msg, kWpaEventScanFailed)) {
        int eventScanId = -1;
        const bool hasScanId = GetWpaEventScanId(msg, eventScanId);
        if (hasScanId) {
            int expectedScanId = eventScanId;
            if (mPendingScanId.compare_exchange_strong(expectedScanId, -1)) {
                LOGW("active scan failed. id=%d", eventScanId);
            }
        } else if (state() == State::Connecting) {
            finishReconnectAttempt();
        }
        return;
    }

    if (starts_with(msg, WPA_EVENT_CONNECTED)) {
        if (mShuttingDown.load()) return;
        setState(State::Connected, "CTRL-EVENT-CONNECTED");

        // 连接已经起来：清理重连抖动控制
        mReconnFailCount.store(0);
        mConnFailCount.store(0);
        finishReconnectAttempt();

        startDhcpWorker();
        return;
    }

    if (starts_with(msg, WPA_EVENT_DISCONNECTED)) {
        stopDhcpWorker();
        setState(State::Disconnected, "CTRL-EVENT-DISCONNECTED");

        // 允许后续重连再次发起，但避免短时间多次 kick
        finishReconnectAttempt();

        // 非人为断开 -> 自动重连
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("disconnected");
        }
        return;
    }

    if (starts_with(msg, WPA_EVENT_TEMP_DISABLED)) {
        finishReconnectAttempt();
        const std::string reason = getWpaEventParam(msg, "reason");
        if (reason == "CONN_FAILED") {
            // 驱动拒绝关联，非密码错误，允许重连
            setState(State::ApNotFound, "SSID TEMP DISABLED (CONN_FAILED)");
            ++mConnFailCount;
            if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
                scheduleReconnect("temp disabled conn failed");
            }
        } else {
            // 密码错误 / 握手失败等，不再自动重连
            setState(State::AuthFailed, "SSID TEMP DISABLED (auth failed?)");
            mConnFailCount.store(0);
        }
        return;
    }

    if (starts_with(msg, WPA_EVENT_ASSOC_REJECT)) {
        setState(State::ApNotFound, "ASSOC REJECT");
        finishReconnectAttempt();
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("assoc reject");
        }
        return;
    }

    if (starts_with(msg, kWpaEventNetworkNotFound)) {
        setState(State::ApNotFound, "NETWORK NOT FOUND");
        finishReconnectAttempt();
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("network not found");
        }
        return;
    }

    // 其他事件：在这里扩展更多解析/日志
#endif // ENABLED(WIFI)
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
    if (now - last < static_cast<uint64_t>(std::max(mOpt.scan_min_interval_ms, 0))) return false;
    mLastScanMs.store(now);
    std::string reply;
    return runCmd("SCAN", &reply);
}

void WifiHal::finishReconnectAttempt() {
    if (mReconnInFlight.exchange(false)) {
        mReconnCv.notify_all();
    }
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
    LOGV("[WifiHal] runCmd: %s", cmd.c_str());

    std::string r;
    bool ok = mWpa.request(cmd, r);
    if (!ok) return false;
    if (reply) *reply = r;

    // wpa_cli 风格：成功一般返回 "OK" 或输出内容；失败返回 "FAIL"
    if (r == "FAIL") return false;
    return true;
}

bool WifiHal::ensureNetworkConfigured(const std::string& ssid, const std::string& psk) {
    if (ssid.empty() || ssid.size() > 32) {
        LOGE("invalid SSID length: %zu", ssid.size());
        return false;
    }

    std::string encodedPsk;
    if (!psk.empty()) {
        const bool rawPsk = psk.size() == 64 && IsHexString(psk);
        if (!rawPsk && (psk.size() < 8 || psk.size() > 63)) {
            LOGE("invalid PSK length: %zu", psk.size());
            return false;
        }
        if (rawPsk) encodedPsk = psk;
        else if (!EncodeWpaQuoted(psk, encodedPsk)) {
            LOGE("PSK contains control characters");
            return false;
        }
    }

    std::string reply;
    if (!runCmd("ADD_NETWORK", &reply)) return false;

    char* end = nullptr;
    errno = 0;
    const long parsedNetId = std::strtol(reply.c_str(), &end, 10);
    if (errno != 0 || end == reply.c_str() || *end != '\0' ||
        parsedNetId < 0 || parsedNetId > std::numeric_limits<int>::max()) {
        LOGE("invalid ADD_NETWORK reply: %s", reply.c_str());
        return false;
    }
    const int newNetId = static_cast<int>(parsedNetId);
    const std::string netId = std::to_string(newNetId);
    auto removeNewNetwork = [&]() { runCmd("REMOVE_NETWORK " + netId, nullptr); };

    // SSID uses the wpa_supplicant hexadecimal representation, so quotes,
    // backslashes and control bytes cannot alter the control command.
    if (!runCmd("SET_NETWORK " + netId + " ssid " + HexEncode(ssid), nullptr)) {
        removeNewNetwork();
        return false;
    }

    if (psk.empty()) {
        if (!runCmd("SET_NETWORK " + netId + " key_mgmt NONE", nullptr)) {
            removeNewNetwork();
            return false;
        }
    } else {
        if (!runCmd("SET_NETWORK " + netId + " psk " + encodedPsk, nullptr)) {
            removeNewNetwork();
            return false;
        }
    }

    // 启用 scan_ssid=1 扫描隐藏 SSID
    runCmd("SET_NETWORK " + netId + " scan_ssid 1", nullptr);

    // 允许同 SSID 多 BSSID 漫游：交给 wpa_supplicant 做 BSS 选择；bgscan 帮助后台扫描
    // 越大越省电/越少扫描
    runCmd("SET_NETWORK " + netId + " bgscan \"simple:120:-67:900\"", nullptr);

    if (!runCmd("ENABLE_NETWORK " + netId, nullptr) ||
        !runCmd("SELECT_NETWORK " + netId, nullptr)) {
        removeNewNetwork();
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(mMtx);
        mLastNetId = newNetId;
    }

    // 本 HAL 只维护一个目标网络。新配置已成功启用后，再清理历史配置，
    // 避免进程重启导致 wpa_supplicant.conf 中的 network 持续累积。
    removeNetworksExcept(newNetId);

    // wpa_supplicant.conf 开启了 update_config=1，SAVE_CONFIG 才有效
    if (!runCmd("SAVE_CONFIG", nullptr)) {
        LOGW("SAVE_CONFIG failed for network id=%d", newNetId);
    }
    return true;
}

void WifiHal::removeNetworksExcept(int keepNetId) {
    std::string reply;
    if (!runCmd("LIST_NETWORKS", &reply)) {
        LOGW("LIST_NETWORKS failed while pruning old networks");
        return;
    }

    std::istringstream iss(reply);
    std::string line;
    bool first = true;
    while (std::getline(iss, line)) {
        if (first) {
            first = false;
            continue;
        }

        const size_t separator = line.find('\t');
        const std::string idText = line.substr(0, separator);
        int networkId = -1;
        if (!ParseNonNegativeInt(idText, networkId) || networkId == keepNetId) {
            continue;
        }

        if (!runCmd("REMOVE_NETWORK " + std::to_string(networkId), nullptr)) {
            LOGW("failed to remove stale network id=%d", networkId);
        }
    }
}

void WifiHal::startDhcpWorker() {
    stopDhcpWorker();
    std::lock_guard<std::mutex> lk(mDhcpLifecycleMtx);
    if (mShuttingDown.load() || mUserDisconnect.load()) return;
    mDhcpStop.store(false);
    mDhcpTh = std::thread(&WifiHal::startDhcp, this);
}

void WifiHal::stopDhcpWorker() {
    std::thread worker;
    {
        std::lock_guard<std::mutex> lk(mDhcpLifecycleMtx);
        mDhcpStop.store(true);
        mDhcpCv.notify_all();
        if (mDhcpTh.joinable()) worker = std::move(mDhcpTh);
    }
    if (worker.joinable()) worker.join();
}

bool WifiHal::runDhcpCommand() {
    const pid_t pid = ::fork();
    if (pid < 0) {
        LOGE("fork DHCP command failed. errno=%d", errno);
        return false;
    }
    if (pid == 0) {
        ::setpgid(0, 0);
        ::execl("/bin/sh", "sh", "-c", mOpt.dhcp_cmd.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }

    // Make the DHCP shell and its descendants independently terminable.
    ::setpgid(pid, pid);
    int status = 0;
    while (true) {
        const pid_t waitResult = ::waitpid(pid, &status, WNOHANG);
        if (waitResult == pid) {
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
        if (waitResult < 0 && errno != EINTR) {
            LOGE("waitpid DHCP command failed. errno=%d", errno);
            return false;
        }

        if (mDhcpStop.load() || mShuttingDown.load()) {
            if (::kill(-pid, SIGTERM) != 0) ::kill(pid, SIGTERM);
            for (int i = 0; i < 5; ++i) {
                if (::waitpid(pid, &status, WNOHANG) == pid) return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (::kill(-pid, SIGKILL) != 0) ::kill(pid, SIGKILL);
            while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) { }
            return false;
        }

        std::unique_lock<std::mutex> lk(mDhcpWaitMtx);
        mDhcpCv.wait_for(lk, std::chrono::milliseconds(100), [this]() {
            return mDhcpStop.load() || mShuttingDown.load();
        });
    }
}

bool WifiHal::startDhcp() {
    const bool commandOk = runDhcpCommand();
    if (mDhcpStop.load() || mShuttingDown.load()) return false;

    // 等待IP出现（最多5秒）
    std::string ip;
    bool ok = false;
    for (int i = 0; commandOk && i < 50; ++i) { // 50 * 100ms = 5s
        if (GetIpv4(mOpt.ifname, ip)) { ok = true; break; }
        std::unique_lock<std::mutex> lk(mDhcpWaitMtx);
        if (mDhcpCv.wait_for(lk, std::chrono::milliseconds(100), [this]() {
            return mDhcpStop.load() || mShuttingDown.load();
        })) return false;
    }

    if (mDhcpStop.load() || mShuttingDown.load()) return false;
    if (commandOk && ok) {
        setState(State::IpReady, std::string("dhcp ok ip=") + ip);
        mReconnFailCount.store(0);
        mReconnInFlight.store(false);
    } else {
        // DHCP 失败
        setState(State::Disconnected,
            commandOk ? "dhcp failed (no ip)" : "dhcp command failed");
        if (mOpt.auto_reconnect && !mUserDisconnect.load()) {
            scheduleReconnect("dhcp failed");
        }
    }
    return commandOk && ok;
}

bool WifiHal::parseScanResults(std::vector<ApInfo>& out) {
    out.clear();

#ifdef PRODUCT_X64
    static const int frequencies[] = {
        2412, 2437, 2462, 5180, 5200, 5220, 5745, 5765, 5785
    };
    std::vector<std::string> ssids = {
        "Ricken_5G", "\\*&?-_!$%^&*()_+", "Office_AP", "Guest_Network",
        "大胆狗贼", "Coffee_Free", "LivingRoom", "IoT_Network",
        "ChinaNet-5G", "CMCC-WEB", "TP-Link_2.4G", "Xiaomi_5G",
        "HUAWEI-Home", "我家WIFI", "IMKK", "kk_frame"
    };

    static thread_local std::mt19937 engine(std::random_device{}());
    std::shuffle(ssids.begin(), ssids.end(), engine);

    std::string connectedSsid;
    if (state() == State::IpReady) {
        std::lock_guard<std::mutex> lk(mMtx);
        connectedSsid = mLastSsid;
    }

    std::uniform_int_distribution<int> countDist(5, 10);
    std::uniform_int_distribution<int> frequencyDist(
        0, static_cast<int>(sizeof(frequencies) / sizeof(frequencies[0])) - 1);
    std::uniform_int_distribution<int> signalDist(-90, -30);
    std::uniform_int_distribution<int> encryptionDist(0, 4);
    std::uniform_int_distribution<int> byteDist(0, 255);
    const size_t targetCount = static_cast<size_t>(countDist(engine));

    auto appendAp = [&](const std::string& ssid) {
        unsigned int mac[6];
        for (auto& byte : mac) byte = static_cast<unsigned int>(byteDist(engine));
        mac[0] = (mac[0] & 0xfcU) | 0x02U; // locally administered unicast address

        char bssid[18];
        std::snprintf(bssid, sizeof(bssid), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        ApInfo ap;
        ap.bssid = bssid;
        ap.ssid = ssid;
        ap.freq = frequencies[frequencyDist(engine)];
        ap.signal = signalDist(engine);
        ap.encrypted = encryptionDist(engine) != 0;
        ap.connected = !connectedSsid.empty() && ssid == connectedSsid;
        out.push_back(ap);
    };

    // Keep the active network visible when subsequent scans randomize the list.
    if (!connectedSsid.empty()) appendAp(connectedSsid);
    for (const auto& ssid : ssids) {
        if (out.size() >= targetCount) break;
        if (ssid != connectedSsid) appendAp(ssid);
    }
    return true;
#else
    // SCAN_RESULTS 输出类似：
    // bssid / frequency / signal level / flags / ssid
    // fa:be:81:c3:dc:30       2462    -40     [WPA2-PSK-CCMP][WPS][ESS][P2P]  Ricken
    std::string reply;
    if (!runCmd("SCAN_RESULTS", &reply)) return false;

    std::istringstream iss(reply);
    std::string line;
    LOGV("[WifiHal] SCAN_RESULTS RAW:\n %s", reply.c_str());

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
        ap.bssid = cols[0];
        ap.freq = std::atoi(cols[1].c_str());
        ap.signal = std::atoi(cols[2].c_str());
        ap.encrypted = (cols[3].find("WPA") != std::string::npos) || (cols[3].find("WEP") != std::string::npos);
        ap.ssid = EncodingUtils::hexEscapes(cols[4]);

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
#endif // PRODUCT_X64
}

void WifiHal::scheduleReconnect(const std::string& reason) {
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
    LOGI("schedule Wi-Fi reconnect. reason=%s", reason.c_str());
    mReconnCv.notify_all();
}

void WifiHal::cancelReconnect() {
    const bool wasRunning = mReconnRunning.exchange(false);
    mReconnInFlight.store(false);
    mReconnCv.notify_all();
    if (wasRunning && mReconnTh.joinable()) mReconnTh.join();
}

bool WifiHal::tryReloadDriver() {
    // 冷却检查
    const uint64_t now = cdroid::SystemClock::uptimeMillis();
    const uint64_t last = mLastDriverReloadMs.load();
    const int64_t cooldownMs = static_cast<int64_t>(
        std::max(mOpt.driver_reload_cooldown_sec, 0)) * 1000;
    if (cooldownMs > 0 && now - last < static_cast<uint64_t>(cooldownMs)) {
        LOGW("driver reload cooldown not met, skip");
        return false;
    }
    mLastDriverReloadMs.store(now);

    // 标记重载进行中，外部 scan/connect 将被拒绝，cancelReconnect 不会阻塞等待
    mDriverReloading.store(true);

    // 可中断 sleep：每 100ms 检查是否被取消，避免 join 长时间阻塞
    auto reloadSleep = [this](int ms) -> bool {
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        while (std::chrono::steady_clock::now() < end) {
            if (!mReconnRunning.load()) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return true;
    };

    LOGW("Wi-Fi driver fault detected, reloading driver...");

    // 1. 停止 DHCP
    stopDhcpWorker();

    // 2. 关闭 wpa_ctrl 通道
    mWpa.stopMonitor();
    mWpa.close();

    // 3. 接口 down
    if (!mOpt.ifdown_cmd.empty()) {
        LOGI("driver reload: ifdown (%s)", mOpt.ifdown_cmd.c_str());
        std::system(mOpt.ifdown_cmd.c_str());
    }

    // 4. 平台回调（WiFi 已关闭）
    {
        Callbacks cb = getCallbacks();
        if (cb.onDriverFault) cb.onDriverFault();
    }

    // 5. 卸载驱动
    if (!mOpt.driver_unload_cmd.empty()) {
        LOGI("driver reload: unloading (%s)", mOpt.driver_unload_cmd.c_str());
        std::system(mOpt.driver_unload_cmd.c_str());
    }

    // 6. 卸载与加载之间留出间隔，避免竞争
    if (!reloadSleep(1000)) {
        mDriverReloading.store(false);
        return false;
    }

    // 7. 加载驱动
    if (!mOpt.driver_load_cmd.empty()) {
        LOGI("driver reload: loading (%s)", mOpt.driver_load_cmd.c_str());
        std::system(mOpt.driver_load_cmd.c_str());
    }

    // 8. 等待驱动初始化
    if (!reloadSleep(2000)) {
        mDriverReloading.store(false);
        return false;
    }

    // 9. 重新启用 WiFi（检查是否已被用户关闭或取消）
    if (mShuttingDown.load() || mUserDisconnect.load() || !mReconnRunning.load()) {
        LOGI("driver reload: WiFi re-enable skipped (cancelled)");
        mDriverReloading.store(false);
        return false;
    }

    if (enable()) {
        LOGI("driver reload complete, WiFi re-enabled");
        mDriverReloading.store(false);
        return true;
    }

    LOGE("driver reload failed: WiFi re-enable failed");
    mDriverReloading.store(false);
    return false;
}

void WifiHal::reconnectThread() {
    std::unique_lock<std::mutex> lk(mReconnMtx);

    while (mReconnRunning.load()) {
        // 保留退避时间；普通事件通知不会绕过等待，关闭时可立即唤醒。
        if (mReconnCv.wait_for(lk, std::chrono::milliseconds(mReconnBackoffMs),
            [this]() { return !mReconnRunning.load(); })) {
            break;
        }
        if (mUserDisconnect.load()) continue;

        // 已关联、已获取 IP 或认证失败时不再发起重连。
        const State currentState = state();
        if (currentState == State::Connected || currentState == State::IpReady ||
            currentState == State::AuthFailed) {
            mReconnBackoffMs = std::max(mOpt.reconnect_initial_ms, 100);
            continue;
        }

        // 驱动异常：连续 CONN_FAILED 达到阈值，尝试重载驱动
        if (mOpt.driver_reload_after_fails > 0 &&
            mConnFailCount.load() >= mOpt.driver_reload_after_fails) {
            LOGW("driver fault: CONN_FAILED count=%d >= threshold=%d",
                 mConnFailCount.load(), mOpt.driver_reload_after_fails);
            mConnFailCount.store(0);
            mReconnInFlight.store(false);
            lk.unlock();
            const bool reloadOk = tryReloadDriver();
            lk.lock();
            if (!mReconnRunning.load()) break;
            if (!reloadOk) {
                LOGE("driver reload failed, abort auto reconnect");
                mReconnRunning.store(false);
                break;
            }

            // 重载后 wpa_supplicant 丢失了运行时网络配置，
            // 必须走完整的 ADD_NETWORK / SET_NETWORK / SELECT_NETWORK 流程
            std::string ssid, psk;
            {
                std::lock_guard<std::mutex> guard(mMtx);
                ssid = mLastSsid;
                psk = mLastPsk;
            }
            lk.unlock();
            setState(State::Connecting, "auto reconnect after driver reload");
            ensureNetworkConfigured(ssid, psk);
            lk.lock();

            mReconnBackoffMs = std::max(mOpt.reconnect_initial_ms, 100);
            continue;
        }

        mReconnInFlight.store(true);

        lk.unlock();
        setState(State::Connecting, "auto reconnect");
        const bool commandAccepted = reconnectLight();
        lk.lock();

        if (!commandAccepted) {
            mReconnInFlight.store(false);
        } else {
            const int timeoutMs = std::max(mOpt.reconnect_attempt_timeout_ms, 1000);
            const bool finished = mReconnCv.wait_for(
                lk, std::chrono::milliseconds(timeoutMs), [this]() {
                    return !mReconnRunning.load() || !mReconnInFlight.load();
            });
            if (!mReconnRunning.load()) break;
            if (!finished) {
                mReconnInFlight.store(false);
                LOGW("Wi-Fi reconnect attempt timed out after %d ms", timeoutMs);
            }
        }

        const State outcomeState = state();
        if (outcomeState == State::Connected || outcomeState == State::IpReady) {
            mReconnFailCount.store(0);
            mReconnBackoffMs = std::max(mOpt.reconnect_initial_ms, 100);
        } else if (outcomeState != State::AuthFailed) {
            int fails = ++mReconnFailCount;
            bool shouldScan = fails >= std::max(mOpt.reconn_fail_before_scan, 1);
            lk.unlock();
            if (shouldScan && state() != State::Connected && state() != State::IpReady) {
                if (maybeScanForReconnect()) {
                    mReconnFailCount.store(0);
                }
            }
            lk.lock();
        }

        if (outcomeState != State::Connected && outcomeState != State::IpReady &&
            outcomeState != State::AuthFailed) {
            const int maxBackoffMs = std::max(mOpt.reconnect_max_ms, 100);
            const int64_t nextBackoffMs = static_cast<int64_t>(mReconnBackoffMs) * 2;
            mReconnBackoffMs = static_cast<int>(
                std::min<int64_t>(nextBackoffMs, maxBackoffMs));
        }
    }

    mReconnInFlight.store(false);
}
