/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-04 14:00:00
 * @FilePath: /kk_frame/src/utils/platform/device_utils.cc
 * @Description: 屏体硬件控制（背光、音量）产品适配层
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "device_utils.h"

#include "path_utils.h"
#include "process_utils.h"
#include "sysnode_utils.h"

#include <cdlog.h>

#include <algorithm>
#include <atomic>
#include <string>
#include <unistd.h>

namespace {

    /// @brief 屏幕背光控制参数
    struct BrightnessConfig {
        bool        available = false;  // 当前平台是否已适配，false 时只打印日志
        std::string root;               // 设备节点根路径
        std::string enablePath;         // 使能/电源节点
        std::string valuePath;          // 亮度节点
        std::string onValue;            // 点亮时写入使能节点的值
        std::string offValue;           // 熄屏时写入使能节点的值
        int         minValue = 0;       // 亮屏状态下亮度节点的最小有效值
        int         maxValue = 100;     // 亮度节点满量程
    };

    /// @brief 获取背光配置（首次调用时初始化，设备节点缺失则标记为未适配）
    const BrightnessConfig& brightnessConfig() {
        static const BrightnessConfig config = []() {
            BrightnessConfig cfg;
#if defined(PRODUCT_SIGMA)
            // duty_cycle 直接为占空比百分比，有效范围 [1-99]，无需调整 period
            cfg.root = "/sys/class/pwm/pwmchip0/pwm2";
            cfg.enablePath = PathUtils::join(cfg.root, "enable");
            cfg.valuePath = PathUtils::join(cfg.root, "duty_cycle");
            cfg.onValue = "1";
            cfg.offValue = "0";
            cfg.minValue = 1;
            cfg.maxValue = 99;
#elif defined(PRODUCT_RK3506)
            // bl_power 为 FB_BLANK 等级：0 点亮，4 熄屏
            cfg.root = "/sys/class/backlight/backlight";
            cfg.enablePath = PathUtils::join(cfg.root, "bl_power");
            cfg.valuePath = PathUtils::join(cfg.root, "brightness");
            cfg.onValue = "0";
            cfg.offValue = "4";
            cfg.minValue = 0;
            const int max = SysNodeUtils::readInt(PathUtils::join(cfg.root, "max_brightness"), 0);
            if (max > 0) cfg.maxValue = max;
#endif
            if (!cfg.valuePath.empty() && access(cfg.root.c_str(), F_OK) == 0) {
                cfg.available = true;
            }
            return cfg;
        }();
        return config;
    }

} // namespace

void DeviceUtils::setBrightness(int value, bool swap) {
    value = std::min(std::max(value, 0), 100);
    if (swap) value = 100 - value;

    const BrightnessConfig& cfg = brightnessConfig();
    if (!cfg.available) {
        LOGI("设置屏幕背光 %d | %d", value, swap);
        return;
    }

    // 去重，避免滑块拖动时频繁写 sysfs
    // 注意：只记住本进程最后一次设置的值，硬件被外部改变时不会自动重写
    static std::atomic<int> lastValue{ -1 };
    if (value == lastValue.load()) return;
    lastValue.store(value);

    if (value) {
        // +50 为四舍五入（补半个除数），避免整数除法截断导致亮度整体偏暗
        int duty = (value * cfg.maxValue + 50) / 100;
        duty = std::min(std::max(duty, cfg.minValue), cfg.maxValue);
        SysNodeUtils::write(cfg.enablePath, cfg.onValue);
        SysNodeUtils::write(cfg.valuePath, std::to_string(duty));
        LOGV("设置屏幕背光 %d", value);
    } else {
        SysNodeUtils::write(cfg.enablePath, cfg.offValue);
        LOGV("关闭屏幕背光");
    }
}

void DeviceUtils::setVolume(int value) {
    value = std::min(std::max(value, 0), 100);

    // 去重，避免拖动音量条时频繁执行命令
    // 注意：同 setBrightness，只反映本进程最后一次设置的值
    static std::atomic<int> lastValue{ -1 };
    if (value == lastValue.load()) return;
    lastValue.store(value);

#if defined(PRODUCT_RK3506)
    // amixer scontrols
    // amixer set 'DAC Digital' 155~255
    // amixer set 'Master' 0~100
    if (value > 0) {
        ProcessUtils::run("amixer set 'Master' 100");
        ProcessUtils::run("amixer set 'DAC Digital' " + std::to_string(155 + value));
    } else {
        ProcessUtils::run("amixer set 'Master' 0");
        ProcessUtils::run("amixer set 'DAC Digital' 0");
    }
    LOGV("设置音量 %d", value);
#else
    LOGI("设置音量 %d", value);
#endif
}
