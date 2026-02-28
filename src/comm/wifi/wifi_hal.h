/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:43:34
 * @LastEditTime: 2026-02-28 14:21:08
 * @FilePath: /kk_frame/src/comm/wifi/wifi_hal.h
 * @Description: WiFi 适配层
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIFI_HAL_H__
#define __WIFI_HAL_H__

#include "wpa_client.h"
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class WifiHal {
public:
    enum class State {
        Off = 0,
        Idle,
        Scanning,
        Connecting,
        Connected,
        IpReady,
        Disconnected,
        AuthFailed,
        ApNotFound
    };

    struct ApInfo {
        std::string bssid;
        std::string ssid;
        int         freq = 0;
        int         signal = -100; // dBm
        bool        encrypted = true;
        bool        connected = false;
    };

    struct Callbacks {
        // 状态变更
        std::function<void(State newState, const std::string& reason)> onStateChanged;

        // 扫描结束通知
        std::function<void(const std::vector<ApInfo>& aps)> onScanDone;
    };

    struct Options {
        std::string ifname = "wlan0";
        std::string ctrl_path = "/var/run/wpa_supplicant";

        // 是否自动重连（上电 + 非人为断开）
        bool auto_reconnect = true;

        // 自动重连退避
        int  reconnect_initial_ms = 2000;
        int  reconnect_max_ms     = 10000;

        // 自动重连失败时触发主动 SCAN
        int scan_min_interval_ms    = 15000; // SCAN最小间隔
        int reconn_fail_before_scan = 3;     // 连续连失败N次后触发SCAN

        // DHCP 命令
        std::string dhcp_cmd   = "udhcpc -i wlan0 -n -q";
        std::string ifup_cmd   = "ifconfig wlan0 up";
        std::string ifdown_cmd = "ifconfig wlan0 down";
    };

    explicit WifiHal(const Options& opt);
    ~WifiHal();

    void setCallbacks(const Callbacks& cb);

    // 打开/关闭 WiFi（这里指：接口 up/down + wpa_ctrl 通道 ready）
    bool enable();
    void disable();

    // 扫描（异步：完成后回调 onScanDone）
    bool scan();

    // 连接（异步：状态变化走 onStateChanged；成功后会跑 DHCP）
    bool connect(const std::string& ssid, const std::string& psk);

    // 人为断开（会抑制 auto reconnect）
    bool disconnect();

    // 获取当前状态（带锁线程安全）
    State state() const;

    // 当前连接信息（从 STATUS 里取）
    bool getStatus(std::string& outStatusText);

private:
    // 获取回调函数（带锁线程安全）
    Callbacks getCallbacks();

    // wpa event
    void onWpaEvent(const std::string& msg);

    // helpers
    void setState(State s, const std::string& reason);
    bool runCmd(const std::string& cmd, std::string* reply = nullptr);

    bool ensureNetworkConfigured(const std::string& ssid, const std::string& psk);
    bool startDhcp();

    bool parseScanResults(std::vector<ApInfo>& out);

    // auto reconnect helpers
    bool reconnectLight();
    bool maybeScanForReconnect();

    // auto reconnect
    void scheduleReconnect(const std::string& reason);
    void cancelReconnect();
    void reconnectThread();

private:
    Options   mOpt;
    Callbacks mCb;

    mutable std::mutex mMtx;
    State mState{ State::Off };

    WpaClient mWpa;

    // 记住上一次“可自动重连”的目标
    std::string       mLastSsid;
    std::string       mLastPsk;
    std::atomic<bool> mUserDisconnect{ false };

    // 允许同 SSID 下切换不同 BSSID：自动重连时不重建网络配置，复用最近一次 netId
    int mLastNetId = -1;

    // 防止重连风暴：同一时刻只允许一次自动重连尝试在飞
    std::atomic<bool>      mReconnInFlight{ false };
    std::atomic<int>       mReconnFailCount{ 0 };
    std::atomic<uint64_t>  mLastScanMs{ 0 };

    // 重连线程
    std::thread             mReconnTh;
    std::atomic<bool>       mReconnRunning{ false };
    std::condition_variable mReconnCv;
    std::mutex              mReconnMtx;
    int                     mReconnBackoffMs = 0;
};

#endif // !__WIFI_HAL_H__