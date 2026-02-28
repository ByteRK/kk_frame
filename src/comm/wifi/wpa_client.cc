/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:39:44
 * @LastEditTime: 2026-02-27 17:13:53
 * @FilePath: /kk_frame/src/comm/wifi/wpa_client.cc
 * @Description: wpa_ctrl 客户端封装
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "wpa_client.h"
#include <cstring>
#include <chrono>
#include <cdlog.h>

static std::string NormalizeWpaEvent(std::string msg) {
    // 去掉开头的 "<3>"
    if (msg.size() >= 3 && msg[0] == '<') {
        auto pos = msg.find('>');
        if (pos != std::string::npos) {
            msg.erase(0, pos + 1);
        }
    }

    // 去掉 "IFNAME=wlan0 " 这类前缀（可能还有 "IFNAME=wlan0 " + "CTRL-EVENT-..."）
    const std::string key = "IFNAME=";
    if (msg.compare(0, key.size(), key) == 0) {
        auto sp = msg.find(' ');
        if (sp != std::string::npos) {
            msg.erase(0, sp + 1);
        }
    }

    // 去掉开头空格
    while (!msg.empty() && (msg[0] == ' ' || msg[0] == '\t')) msg.erase(0, 1);
    return msg;
}

WpaClient::WpaClient(const Options& opt) : mOpt(opt) {

}

WpaClient::~WpaClient() {
    stopMonitor();
    close();
}

bool WpaClient::open() {
    if (mCmd) return true;

    // ctrl interface socket path 通常是: /var/run/wpa_supplicant/wlan0
    const std::string sock = mOpt.ctrl_path + "/" + mOpt.ifname;

    mCmd = wpa_ctrl_open(sock.c_str());
    if (!mCmd) return false;

    mMon = wpa_ctrl_open(sock.c_str());
    if (!mMon) {
        wpa_ctrl_close(mCmd);
        mCmd = nullptr;
        return false;
    }
    return true;
}

void WpaClient::close() {
    if (mMon) {
        wpa_ctrl_close(mMon);
        mMon = nullptr;
    }
    if (mCmd) {
        wpa_ctrl_close(mCmd);
        mCmd = nullptr;
    }
}

bool WpaClient::request(const std::string& cmd, std::string& reply) {
    reply.clear();
    if (!mCmd) return false;

    std::lock_guard<std::mutex> lk(mCmdMtx);

    char buf[4096];
    std::memset(buf, 0, sizeof(buf));
    size_t len = sizeof(buf) - 1;

    int ret = wpa_ctrl_request(
        mCmd,
        cmd.c_str(),
        cmd.size(),
        buf,
        &len,
        nullptr
    );

    if (ret != 0) {
        return false;
    }

    // reply 末尾可能带 '\n'
    reply.assign(buf, len);
    while (!reply.empty() && (reply.back() == '\n' || reply.back() == '\r')) {
        reply.pop_back();
    }
    return true;
}

bool WpaClient::startMonitor(const EventCallback& cb) {
    if (!mMon) return false;
    if (mMonRunning.load()) return true;

    mEventCb = cb;

    if (wpa_ctrl_attach(mMon) != 0) {
        return false;
    }

    mMonRunning.store(true);
    mMonThread = std::thread(&WpaClient::monitorLoop, this);
    return true;
}

void WpaClient::stopMonitor() {
    if (!mMonRunning.exchange(false)) return;

    if (mMon) {
        wpa_ctrl_detach(mMon);
        wpa_ctrl_close(mMon);   // 关键：打断阻塞 recv
        mMon = nullptr;
    }

    if (mMonThread.joinable()) {
        mMonThread.join();
    }
}

void WpaClient::monitorLoop() {
    while (mMonRunning.load()) {
        char buf[4096];
        std::memset(buf, 0, sizeof(buf));
        size_t len = sizeof(buf) - 1;

        // wpa_ctrl_recv 是阻塞的：如果你希望可中断，可以用 select/poll（这里先保持简洁）
        int ret = wpa_ctrl_recv(mMon, buf, &len);
        if (ret == 0 && len > 0) {
            LOG(VERBOSE) << "WPA-EVENT(raw): " << std::string(buf, len);
            std::string msg(buf, len);
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
                msg.pop_back();
            }
            if (mEventCb) mEventCb(NormalizeWpaEvent(msg));
        } else {
            // 避免异常忙等
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}