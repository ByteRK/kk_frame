/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:39:44
 * @LastEditTime: 2026-02-27 17:13:49
 * @FilePath: /kk_frame/src/comm/wifi/wpa_client.h
 * @Description: wpa 客户端封装
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WPA_CLIENT_H__
#define __WPA_CLIENT_H__

#include <string>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>

extern "C" {
#include <wpa_ctrl.h>
}

class WpaClient {
public:
    using EventCallback = std::function<void(const std::string& msg)>;

    struct Options {
        std::string ctrl_path = "/var/run/wpa_supplicant";
        std::string ifname    = "wlan0";
        int         cmd_timeout_ms = 2000;
    };

    explicit WpaClient(const Options& opt);
    ~WpaClient();

    bool open();
    void close();

    // 发送命令（线程安全）
    // 返回 true 表示 wpa_ctrl_request 成功（不代表业务成功，比如返回 "FAIL" 也算成功收到回复）
    bool request(const std::string& cmd, std::string& reply);

    // 启动/停止事件监听（CTRL-EVENT-*）
    bool startMonitor(const EventCallback& cb);
    void stopMonitor();

    bool isOpen() const { return mCmd != nullptr; }

private:
    void monitorLoop();

private:
    Options   mOpt;                          // 配置项

    wpa_ctrl* mCmd = nullptr;                // 命令通道
    wpa_ctrl* mMon = nullptr;                // 事件通道

    std::mutex mCmdMtx;                      // 命令通道互斥锁

    std::thread       mMonThread;            // 事件监听线程
    std::atomic<bool> mMonRunning{ false };  // 事件监听线程是否在运行
    EventCallback     mEventCb;              // 事件回调
};

#endif // !__WPA_CTRL_CLIENT_H__