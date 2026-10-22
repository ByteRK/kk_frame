/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-22 11:23:57
 * @LastEditTime: 2026-10-22 17:29:08
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
/// @param rebootTime 判定时间，已运行时间达到该值时触发重启回调
AutoReboot::AutoReboot(int64_t rebootTime) :
    mRebootTime(rebootTime) {
    mTicker.setTick(kTickIntervalMs);
    mTicker.setCallBack([this](int64_t nowMs) { onAutoRebootTick(nowMs); });
}

/// @brief 析构
AutoReboot::~AutoReboot() {
    stopAutoReboot();
}

/// @brief 开始检测
/// @param delayMs 首次检测延迟（毫秒）：-1 立即检测，0 下一轮事件循环检测，>0 延迟指定毫秒
void AutoReboot::startAutoReboot(int64_t delayMs /* = 0 */) {
    if (mIsRebooted) {
        LOGW("AutoReboot(%p): already rebooted, start ignored", this);
        return;
    }
    if (mRebootTime <= 0) {
        LOGW("AutoReboot(%p): invalid reboot time(%lld), start ignored",
            this, static_cast<long long>(mRebootTime));
        return;
    }

    // 已在检测中：按本次参数重新排定，避免沿用之前的长睡计划
    if (mIsRunning) {
        rescheduleAutoReboot(delayMs);
        return;
    }

    applyAutoRebootInterval(kTickIntervalMs);
    mTicker.startTick(delayMs);
    mIsRunning = true;
}

/// @brief 停止检测
void AutoReboot::stopAutoReboot() {
    mTicker.stopTick();
    mIsRunning = false;
}

/// @brief 是否正在检测
/// @return true 正在检测
bool AutoReboot::isAutoRebootRunning() const {
    return mIsRunning;
}

/// @brief 是否已触发过重启
/// @return true 已触发
bool AutoReboot::isAutoRebooted() const {
    return mIsRebooted;
}

/// @brief 设置判定时间
/// @param rebootTime 判定时间（毫秒）
void AutoReboot::setAutoRebootTime(int64_t rebootTime) {
    if (mIsRebooted) {
        LOGW("AutoReboot(%p): already rebooted, setRebootTime ignored", this);
        return;
    }
    if (rebootTime <= 0) {
        LOGW("AutoReboot(%p): invalid reboot time(%lld), setRebootTime ignored",
            this, static_cast<long long>(rebootTime));
        return;
    }
    if (rebootTime == mRebootTime) return;

    mRebootTime = rebootTime;
    mLastWaitLogMs = 0;  // 新阈值重新开始日志限频

    // 定时器可能正长睡（睡到旧阈值或因上层返回的 nextMs），
    // 不重排会沿用旧计划，导致新阈值要等旧计划触发后才生效
    if (mInTick) {
        mRebootTimeDirty = true;
        return;
    }
    if (mIsRunning) rescheduleAutoReboot(0);
}

/// @brief 获取判定时间
/// @return 判定时间
int64_t AutoReboot::getAutoRebootTime() const {
    return mRebootTime;
}

/// @brief 应用检测间隔
/// @param intervalMs 下次检测间隔
void AutoReboot::applyAutoRebootInterval(int64_t intervalMs) {
    if (intervalMs <= 0) intervalMs = kTickIntervalMs;
    if (intervalMs < kTickIntervalMs) intervalMs = kTickIntervalMs;
    if (intervalMs == mIntervalMs) return;

    mIntervalMs = intervalMs;
    mTicker.setTick(mIntervalMs);
}

/// @brief 重新排定检测
/// @param delayMs 首次检测延迟（毫秒）：-1 立即检测，0 下一轮事件循环检测，>0 延迟指定毫秒
/// @note 先摘除可能长睡的旧计划，再按默认周期重新注册，确保新判定时间立即生效
void AutoReboot::rescheduleAutoReboot(int64_t delayMs) {
    mTicker.stopTick();
    mIntervalMs = kTickIntervalMs;
    mTicker.setTick(kTickIntervalMs);
    mTicker.startTick(delayMs);
    mIsRunning = true;
}

/// @brief 定时器回调
/// @param nowMs 当前时间戳（毫秒）
void AutoReboot::onAutoRebootTick(int64_t nowMs) {
    mInTick = true;
    mRebootTimeDirty = false;

    const int64_t uptime = getAutoRebootUptime();
    if (uptime < 0) {
        mInTick = false;
        LOGW("AutoReboot(%p): get uptime failed, retry next tick", this);
        return;
    }

    // 未达判定时间：直接睡到阈值时刻
    if (uptime < mRebootTime) {
        mInTick = false;
        applyAutoRebootInterval(mRebootTime - uptime);
        if (mRebootTimeDirty) rescheduleAutoReboot(0);
        return;
    }

    // 已达判定时间：首次打印日志
    if (mLastWaitLogMs == 0) {
        LOGI("AutoReboot(%p): uptime(%lld) reached reboot time(%lld), triggering reboot",
            this, static_cast<long long>(uptime), static_cast<long long>(mRebootTime));
    }

    // 获取判断结果
    const AutoRebootResult result = checkAutoReboot();
    mInTick = false;

    // 条件满足
    if (result.reboot) {
        stopAutoReboot();
        mIsRebooted = true;
        return;
    }

    // 判定过程中修改了阈值：本次结果基于旧阈值，丢弃并按新阈值重排
    if (mRebootTimeDirty) {
        mRebootTimeDirty = false;
        rescheduleAutoReboot(0);
        return;
    }

    // 条件不满足的日志限频，避免逐秒刷屏
    if (mLastWaitLogMs == 0 || nowMs - mLastWaitLogMs >= kWaitLogIntervalMs) {
        mLastWaitLogMs = nowMs;
        LOGW("AutoReboot(%p): reboot is not allowed now, retry later", this);
    }
    applyAutoRebootInterval(result.nextMs);
}
