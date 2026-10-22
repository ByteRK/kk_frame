/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-22 11:23:57
 * @LastEditTime: 2026-10-22 11:43:58
 * @FilePath: /kk_frame/src/class/auto_reboot.cc
 * @Description: 自动重启检测类
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "auto_reboot.h"

#include <cdlog.h>

/// @brief 构造
/// @param rebootTime 判定时间，已运行时间达到该值时触发重启
AutoReboot::AutoReboot(int64_t rebootTime) :
    mRebootTime(rebootTime) {
}

/// @brief 析构
AutoReboot::~AutoReboot() {
    stop();
}

/// @brief 开始检测
/// @param delayMs 首次检测延迟（毫秒）：-1 立即检测，0 下一轮事件循环检测，>0 延迟指定毫秒
void AutoReboot::start(int64_t delayMs /* = 0 */) {
    if (mIsRebooted) {
        LOGW("AutoReboot(%p): already rebooted, start ignored", this);
        return;
    }
    if (mRebootTime <= 0) {
        LOGW("AutoReboot(%p): invalid reboot time(%lld), start ignored",
            this, static_cast<long long>(mRebootTime));
        return;
    }
    setTick(kTickIntervalMs);
    startTick(delayMs);
    mIsRunning = true;
}

/// @brief 停止检测
void AutoReboot::stop() {
    stopTick();
    mIsRunning = false;
}

/// @brief 是否正在检测
/// @return true 正在检测
bool AutoReboot::isRunning() const {
    return mIsRunning;
}

/// @brief 是否已触发过重启
/// @return true 已触发
bool AutoReboot::isRebooted() const {
    return mIsRebooted;
}

/// @brief 设置判定时间
/// @param rebootTime 判定时间，单位需与 getUptime() 一致
void AutoReboot::setRebootTime(int64_t rebootTime) {
    if (mIsRebooted) {
        LOGW("AutoReboot(%p): already rebooted, setRebootTime ignored", this);
        return;
    }
    mRebootTime = rebootTime;
}

/// @brief 获取判定时间
/// @return 判定时间
int64_t AutoReboot::getRebootTime() const {
    return mRebootTime;
}

/// @brief Tick 回调，固定 1 秒一次
/// @param nowMs 当前时间戳（毫秒）
void AutoReboot::onTick(int64_t nowMs) {
    const int64_t uptime = getUptime();
    if (uptime < 0) {
        LOGW("AutoReboot(%p): getUptime failed, retry next tick", this);
        return;
    }
    if (uptime < mRebootTime) return;

    // 上层可能暂时无法重启，此时保持检测，等下个检测周期再试
    const bool firstTime = (mLastWaitLogMs == 0);
    if (firstTime) {
        LOGI("AutoReboot(%p): uptime(%lld) reached reboot time(%lld), triggering reboot",
            this, static_cast<long long>(uptime), static_cast<long long>(mRebootTime));
    }
    if (!onReboot()) {
        if (firstTime || nowMs - mLastWaitLogMs >= kWaitLogIntervalMs) {
            mLastWaitLogMs = nowMs;
            LOGW("AutoReboot(%p): reboot is not allowed now, retry in next tick", this);
        }
        return;
    }

    // 确认已重启，停止检测并置位，避免重复回调
    stop();
    mIsRebooted = true;
}
