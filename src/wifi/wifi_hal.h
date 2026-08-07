/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:43:34
 * @LastEditTime: 2026-08-07 09:28:31
 * @FilePath: /kk_frame/src/wifi/wifi_hal.h
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
#include <memory>

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

        // 驱动异常通知（WiFi已关闭，平台可做自定义处理）
        std::function<void()> onDriverFault;
    };

    struct Options {
        std::string ifname = "wlan0";
        std::string ctrl_path = "/var/run/wpa_supplicant";

        // 是否自动重连（上电 + 非人为断开）
        bool auto_reconnect = true;

        // 自动重连退避
        int  reconnect_initial_ms = 2000;
        int  reconnect_max_ms     = 10000;
        int  reconnect_attempt_timeout_ms = 15000;

        // 自动重连失败时触发主动 SCAN
        int scan_min_interval_ms    = 15000; // SCAN最小间隔
        int reconn_fail_before_scan = 3;     // 连续连失败N次后触发SCAN

        // x64 模拟操作的返回延迟
        int x64_scan_delay_ms    = 1000;
        int x64_connect_delay_ms = 1000;

        // DHCP 命令
        std::string dhcp_cmd   = "udhcpc -i wlan0 -n -q";
        std::string ifup_cmd   = "ifconfig wlan0 up";
        std::string ifdown_cmd = "ifconfig wlan0 down";

        // 驱动故障自动恢复（CONN_FAILED 连续出现时触发驱动重载）
        std::string driver_unload_cmd;                  // 卸载驱动命令（如 rmmod rtl88x2bu），为空则跳过
        std::string driver_load_cmd;                    // 加载驱动命令（如 modprobe rtl88x2bu），为空则跳过
        int         driver_reload_after_fails  = 0;     // 连续 CONN_FAILED 次数阈值，0 = 禁用
        int         driver_reload_cooldown_sec = 300;   // 两次重载最小间隔（秒）
    };

    explicit WifiHal(const Options& opt);
    ~WifiHal();

    void setCallbacks(const Callbacks& cb);

    // 打开/关闭 WiFi（这里指：接口 up/down + wpa_ctrl 通道 ready）
    bool enable();
    void disable();

    // 主动扫描（异步：仅本次主动扫描完成后回调 onScanDone）
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
#ifdef PRODUCT_X64
    void onX64ScanResultTimer();
    void onX64ConnectResultTimer();
#endif

    // helpers
    void setState(State s, const std::string& reason);
    bool runCmd(const std::string& cmd, std::string* reply = nullptr);

    bool ensureNetworkConfigured(const std::string& ssid, const std::string& psk);
    void startDhcpWorker();
    void stopDhcpWorker();
    bool runDhcpCommand();
    bool startDhcp();

    bool parseScanResults(std::vector<ApInfo>& out);
    void removeNetworksExcept(int keepNetId);

    // auto reconnect helpers
    bool reconnectLight();
    bool maybeScanForReconnect();
    void finishReconnectAttempt();

    // auto reconnect
    void scheduleReconnect(const std::string& reason);
    void cancelReconnect();
    void reconnectThread();

    // driver recovery
    bool tryReloadDriver();

private:
#ifdef PRODUCT_X64
    class X64DelayTimer;
#endif

    Options   mOpt;
    Callbacks mCb;

    mutable std::mutex mMtx;
    State mState{ State::Off };

    WpaClient mWpa;

#ifdef PRODUCT_X64
    std::unique_ptr<X64DelayTimer> mX64ScanResultTimer;
    std::unique_ptr<X64DelayTimer> mX64ConnectResultTimer;
#endif

    // 记住上一次“可自动重连”的目标
    std::string       mLastSsid;
    std::string       mLastPsk;
    std::atomic<bool> mUserDisconnect{ false };
    std::atomic<bool> mShuttingDown{ false };

    // DHCP worker：禁止 detached，关闭/析构前必须 join。
    std::thread             mDhcpTh;
    std::atomic<bool>       mDhcpStop{ false };
    std::condition_variable mDhcpCv;
    std::mutex              mDhcpWaitMtx;
    std::mutex              mDhcpLifecycleMtx;

    // 允许同 SSID 下切换不同 BSSID：自动重连时不重建网络配置，复用最近一次 netId
    int mLastNetId = -1;

    // 防止重连风暴：同一时刻只允许一次自动重连尝试在飞
    std::atomic<bool>      mReconnInFlight{ false };
    std::atomic<int>       mReconnFailCount{ 0 };
    std::atomic<uint64_t>  mLastScanMs{ 0 };

    // SCAN use_id=1 返回的 ID；用于过滤 bgscan 和其他来源的扫描事件。
    std::atomic<int>       mPendingScanId{ -1 };

    // 重连线程
    std::thread             mReconnTh;
    std::atomic<bool>       mReconnRunning{ false };
    std::condition_variable mReconnCv;
    std::mutex              mReconnMtx;
    int                     mReconnBackoffMs = 0;

    // 驱动故障检测
    std::atomic<int>        mConnFailCount{ 0 };        // 连续 CONN_FAILED 计数
    std::atomic<uint64_t>   mLastDriverReloadMs{ 0 };   // 上次重载时间戳（毫秒）
    std::atomic<bool>       mDriverReloading{ false };  // 驱动重载进行中，拒绝外部操作
};

#endif // !__WIFI_HAL_H__
