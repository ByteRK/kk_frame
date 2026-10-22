/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-22 11:23:57
 * @LastEditTime: 2026-10-22 16:32:57
 * @FilePath: /kk_frame/src/class/auto_reboot.h
 * @Description: 自动重启检测类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __AUTO_REBOOT_H__
#define __AUTO_REBOOT_H__

#include <stdint.h>

#include "tick_mgr.h"

/// @brief 自动重启检测类
/// @note 可在 checkAutoReboot() 中返回较长间隔把下次检测安排到条件可能满足的时刻
/// @note 判定时间(mRebootTime)、已运行时间(getAutoRebootUptime)均为毫秒
class AutoReboot {
public:
    static const int64_t kTickIntervalMs = 1000;      // 默认检测周期（毫秒）
    static const int64_t kWaitLogIntervalMs = 60000;  // 无法重启时的日志间隔（毫秒）

public:
    /// @brief 检测结果
    struct AutoRebootResult {
        bool    reboot{ false };  // 是否即将重启
        int64_t nextMs{ 0 };      // 下次检测间隔（毫秒），<=0 表示默认周期
    };

private:
    TickMgr::ITickVariable mTicker;         // 检测定时器
    int64_t mRebootTime{ 0 };               // 判定时间（毫秒）
    int64_t mIntervalMs{ kTickIntervalMs }; // 当前检测间隔（毫秒）
    int64_t mLastWaitLogMs{ 0 };            // 上次打印“暂无法重启”日志的时间（毫秒）
    bool    mIsRunning{ false };            // 是否正在检测
    bool    mIsRebooted{ false };           // 是否已触发重启
    bool    mInTick{ false };               // 是否处于检测回调中
    bool    mRebootTimeDirty{ false };      // 判定时间是否在检测回调中被修改

public:
    explicit AutoReboot(int64_t rebootTime);
    virtual ~AutoReboot();

    void    startAutoReboot(int64_t delayMs = 0);
    void    stopAutoReboot();
    bool    isAutoRebootRunning() const;
    bool    isAutoRebooted() const;
    void    setAutoRebootTime(int64_t rebootTime);
    int64_t getAutoRebootTime() const;

protected:
    virtual int64_t          getAutoRebootUptime() = 0;  // 获取已运行时间（毫秒）
    virtual AutoRebootResult checkAutoReboot() = 0;      // 重启回调

private:
    void onAutoRebootTick(int64_t nowMs);
    void applyAutoRebootInterval(int64_t intervalMs);
    void rescheduleAutoReboot(int64_t delayMs);
};

#endif // !__AUTO_REBOOT_H__
