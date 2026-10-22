/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-22 11:23:57
 * @LastEditTime: 2026-10-22 11:44:15
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
/// @note 上层继承本类，构造时传入判定时间，并实现 getUptime()、onReboot() 两个纯虚函数
/// @note 判定时间与 getUptime() 的返回值单位必须一致（建议使用秒）
/// @note 构造完成后需调用 start() 才开始检测
class AutoReboot : public TickMgr::ITickClass {
public:
    /// @brief 检测周期（毫秒）
    static const int64_t kTickIntervalMs = 1000;
    /// @brief 无法重启时的日志间隔（毫秒），避免逐秒刷日志
    static const int64_t kWaitLogIntervalMs = 60000;

private:
    int64_t mRebootTime{ 0 };      // 判定时间
    int64_t mLastWaitLogMs{ 0 };   // 上次打印“暂无法重启”日志的时间
    bool    mIsRunning{ false };   // 是否正在检测
    bool    mIsRebooted{ false };  // 是否已触发重启

public:
    /// @brief 构造
    /// @param rebootTime 判定时间，已运行时间达到该值时触发重启
    explicit AutoReboot(int64_t rebootTime);
    virtual ~AutoReboot();

    /// @brief 开始检测
    /// @param delayMs 首次检测延迟（毫秒）：-1 立即检测，0 下一轮事件循环检测，>0 延迟指定毫秒
    /// @note 已触发过重启则调用无效
    void start(int64_t delayMs = 0);

    /// @brief 停止检测
    void stop();

    /// @brief 是否正在检测
    /// @return true 正在检测
    bool isRunning() const;

    /// @brief 是否已触发过重启
    /// @return true 已触发
    bool isRebooted() const;

    /// @brief 设置判定时间
    /// @param rebootTime 判定时间，单位需与 getUptime() 一致
    /// @note 已触发过重启则调用无效
    void setRebootTime(int64_t rebootTime);

    /// @brief 获取判定时间
    /// @return 判定时间
    int64_t getRebootTime() const;

protected:
    /// @brief 获取设备已运行时间（由上层实现）
    /// @return 已运行时间，单位需与构造传入的判定时间一致；返回负数表示获取失败，本次忽略
    virtual int64_t getUptime() = 0;

    /// @brief 执行重启（由上层实现）
    /// @return true 已执行重启（随后停止检测，不再回调）；false 当前无法重启，保持检测并在下个检测周期重试
    virtual bool onReboot() = 0;

    /// @brief Tick 回调，固定 1 秒一次
    /// @param nowMs 当前时间戳（毫秒）
    void onTick(int64_t nowMs) override;
};

#endif // !__AUTO_REBOOT_H__
