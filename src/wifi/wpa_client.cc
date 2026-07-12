/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 09:39:44
 * @LastEditTime: 2026-03-09 16:24:34
 * @FilePath: /kk_frame/src/wifi/wpa_client.cc
 * @Description: wpa_ctrl 客户端封装
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wpa_client.h"
#include <cstring>
#include <chrono>
#include <cerrno>
#include <poll.h>
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
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    std::lock_guard<std::mutex> lk(mCmdMtx);
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
#else
    LOGI("WpaClient::open()");
#endif // PRODUCT_X64
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

void WpaClient::close() {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    stopMonitor();
    std::lock_guard<std::mutex> lk(mCmdMtx);
    if (mMon) {
        wpa_ctrl_close(mMon);
        mMon = nullptr;
    }
    if (mCmd) {
        wpa_ctrl_close(mCmd);
        mCmd = nullptr;
    }
#else
    LOGI("WpaClient::close()");
#endif // PRODUCT_X64
#else
    LOGE("WIFI is not enabled");
#endif // ENABLED(WIFI)
}

bool WpaClient::request(const std::string& cmd, std::string& reply) {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    reply.clear();
    std::lock_guard<std::mutex> lk(mCmdMtx);
    if (!mCmd) return false;

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
#else
    reply = "OK";
    LOGI("WpaClient::request(%s)", cmd.c_str());
#endif // PRODUCT_X64
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

bool WpaClient::startMonitor(const EventCallback& cb) {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    if (!mMon) return false;
    if (mMonRunning.load()) return true;

    mEventCb = cb;

    if (wpa_ctrl_attach(mMon) != 0) {
        return false;
    }

    mMonRunning.store(true);
    mMonThread = std::thread(&WpaClient::monitorLoop, this);
#else
    LOGI("WpaClient::startMonitor()");
#endif // PRODUCT_X64
    return true;
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

void WpaClient::stopMonitor() {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    mMonRunning.store(false);

    if (mMonThread.joinable()) {
        mMonThread.join();
    }
    if (mMon) wpa_ctrl_detach(mMon);
    mEventCb = nullptr;
#else
    LOGI("WpaClient::stopMonitor()");
#endif // PRODUCT_X64
#else
    LOGE("WIFI is not enabled");
#endif // ENABLED(WIFI)
}

bool WpaClient::isOpen() const {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    std::lock_guard<std::mutex> lk(mCmdMtx);
    return mCmd != nullptr;
#else
    return true;
#endif
#else
    LOGE("WIFI is not enabled");
    return false;
#endif // ENABLED(WIFI)
}

void WpaClient::monitorLoop() {
#if ENABLED(WIFI)
#ifndef PRODUCT_X64
    wpa_ctrl* const monitor = mMon;
    if (!monitor) return;
    const int monitorFd = wpa_ctrl_get_fd(monitor);
    if (monitorFd < 0) return;

    while (mMonRunning.load()) {
        struct pollfd pfd;
        pfd.fd = monitorFd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        const int pollResult = ::poll(&pfd, 1, 200);
        if (!mMonRunning.load()) break;
        if (pollResult == 0) continue;
        if (pollResult < 0) {
            if (errno == EINTR) continue;
            LOGE("WPA monitor poll failed. errno=%d", errno);
            break;
        }
        if ((pfd.revents & POLLIN) == 0) {
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LOGE("WPA monitor fd failed. revents=0x%x", pfd.revents);
                break;
            }
            continue;
        }

        char buf[4096];
        std::memset(buf, 0, sizeof(buf));
        size_t len = sizeof(buf) - 1;

        int ret = wpa_ctrl_recv(monitor, buf, &len);
        if (ret == 0 && len > 0) {
            LOG(VERBOSE) << "WPA-EVENT(raw): " << std::string(buf, len);
            std::string msg(buf, len);
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
                msg.pop_back();
            }
            if (mEventCb) mEventCb(NormalizeWpaEvent(msg));
        }
    }
#endif // PRODUCT_X64
#endif // ENABLED(WIFI)
}
