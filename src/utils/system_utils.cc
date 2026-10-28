/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:26
 * @LastEditTime: 2026-10-28 14:20:20
 * @FilePath: /kk_frame/src/utils/system_utils.cc
 * @Description: 系统相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "system_utils.h"
#include "project_utils.h"
#include "time_utils.h"
#include "string_utils.h"
#include "series_info.h"
#include "config_info.h"
#include <cdlog.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#define TIME_CACHE_FILE "timeCache"

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
            cfg.enablePath = cfg.root + "/enable";
            cfg.valuePath = cfg.root + "/duty_cycle";
            cfg.onValue = "1";
            cfg.offValue = "0";
            cfg.minValue = 1;
            cfg.maxValue = 99;
#elif defined(PRODUCT_RK3506)
            // bl_power 为 FB_BLANK 等级：0 点亮，4 熄屏
            cfg.root = "/sys/class/backlight/backlight";
            cfg.enablePath = cfg.root + "/bl_power";
            cfg.valuePath = cfg.root + "/brightness";
            cfg.onValue = "0";
            cfg.offValue = "4";
            cfg.minValue = 0;
            int max = SystemUtils::readSysfsInt(cfg.root + "/max_brightness", 0);
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

void SystemUtils::reboot() {
    ProjectUtils::saveTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
    sync();
#ifndef PRODUCT_X64
    std::system("reboot");
#else
    ::exit(0);
#endif
}

void SystemUtils::exit() {
    ProjectUtils::saveTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
    sync();
    ::exit(0);
}

void SystemUtils::sync() {
#ifndef PRODUCT_X64
    std::system("sync");
#else
    LOGI("-------- sync --------");
#endif
}

std::string SystemUtils::system(const std::string& cmd) {
    std::string result;
    char        buffer[128]; // 存储输出数据的缓冲区
    int64_t     start = TimeUtils::getTimeMSec();

    // 执行系统命令，并读取输出数据
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == NULL) {
        perror("Failed to execute the command.\n");
        return "";
    }

    // 逐行读取输出数据并打印
    bzero(buffer, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        result.append(buffer);
        bzero(buffer, sizeof(buffer));
    }

    // 关闭指针
    pclose(fp);

    // 裁剪尾部无效内容
    result = StringUtils::trimRight(result);

    // 计算执行时间
    int64_t diff = TimeUtils::getTimeMSec() - start;
    if (diff > 10) LOGW("consume [%lld]ms system [%s] [%s]", diff, cmd.c_str(), result.c_str());
    else LOGV("consume [%lld]ms system [%s] [%s]", diff, cmd.c_str(), result.c_str());
    return result;
}

bool SystemUtils::asyncSystem(const std::string& cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        // fork() 失败
        return false;
    } else if (pid == 0) {
        // 子进程
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        ::_exit(127); // 如果 execl 失败
    }
    // 父进程，fork() 成功
    return true;
}

bool SystemUtils::sysfs(const std::string& path, const std::string& value) {
    FILE* fp = fopen(path.c_str(), "w");
    if (!fp) {
        LOGE("Failed to open %s", path);
        return false;
    }
    bool success = (fprintf(fp, "%s", value.c_str()) > 0);
    fclose(fp);

    if (!success) {
        LOGE("Write failed: %s -> %s", value.c_str(), path.c_str());
    } else {
        LOGV("Write %s -> %s", value.c_str(), path.c_str());
    }
    return success;
}

int SystemUtils::readSysfsInt(const std::string& path, int def) {
    FILE* fp = fopen(path.c_str(), "r");
    if (!fp) {
        LOGE("Failed to open %s", path.c_str());
        return def;
    }
    int value = def;
    if (fscanf(fp, "%d", &value) != 1) {
        LOGE("Read failed: %s", path.c_str());
        value = def;
    } else {
        LOGV("Read %d <- %s", value, path.c_str());
    }
    fclose(fp);
    return value;
}

void SystemUtils::setBrightness(int value, bool swap) {
    value = std::min(std::max(value, 0), 100);
    if (swap) value = 100 - value;

    const BrightnessConfig& cfg = brightnessConfig();
    if (!cfg.available) {
        LOGI("设置屏幕背光 %d | %d", value, swap);
        return;
    }

    // 去重，避免滑块拖动时频繁写 sysfs
    static int lastValue = -1;
    if (value == lastValue) return;
    lastValue = value;

    if (value) {
        // +50 为四舍五入（补半个除数），避免整数除法截断导致亮度整体偏暗
        int duty = (value * cfg.maxValue + 50) / 100;
        duty = std::min(std::max(duty, cfg.minValue), cfg.maxValue);
        sysfs(cfg.enablePath, cfg.onValue);
        sysfs(cfg.valuePath, std::to_string(duty));
        LOGV("设置屏幕背光 %d", value);
    } else {
        sysfs(cfg.enablePath, cfg.offValue);
        LOGV("关闭屏幕背光");
    }
}

void SystemUtils::setVolume(int value) {
    value = std::min(std::max(value, 0), 100);

    // 去重，避免拖动音量条时频繁执行命令
    static int lastValue = -1;
    if (value == lastValue) return;
    lastValue = value;

#if defined(PRODUCT_RK3506)
    // amixer scontrols
    // amixer set 'DAC Digital' 155~255
    // amixer set 'Master' 0~100
    if (value > 0) {
        system("amixer set 'Master' 100");
        system("amixer set 'DAC Digital' " + std::to_string(155 + value));
    } else {
        system("amixer set 'Master' 0");
        system("amixer set 'DAC Digital' 0");
    }
    LOGV("设置音量 %d", value);
#else
    LOGI("设置音量 %d", value);
#endif
}

void SystemUtils::recoverTime() {
    ProjectUtils::loadTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
}

void SystemUtils::clearTimeCache() {
    std::string cacheFile = std::string(APP_DATA_DIR) + TIME_CACHE_FILE;
    system(std::string("rm -f ") + cacheFile);
    sync();
}

void SystemUtils::setTime(const int64_t & timestamp) {
#ifndef PRODUCT_X64
    timeval setTv = {};
    setTv.tv_sec = static_cast<time_t>(timestamp);
    setTv.tv_usec = 0;

    if (settimeofday(&setTv, nullptr) != 0) {
        LOGE("settimeofday failed, timestamp=%lld errno=%d",
            static_cast<long long>(timestamp), errno);
        return;
    }

    syncHWClock();
    LOGE("set time %s", TimeUtils::getTimeFmtStr(static_cast<time_t>(timestamp), "%Y-%m-%d %H:%M:%S").c_str());
#else
    LOGE("set time %s", TimeUtils::getTimeFmtStr(static_cast<time_t>(timestamp), "%Y-%m-%d %H:%M:%S").c_str());
#endif
}

void SystemUtils::setTime(int year, int month, int day, int hour, int minute, int second) {
    if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
        return;
    }

    const std::time_t currentTime = TimeUtils::getTimeSec();
    if (currentTime == static_cast<std::time_t>(-1)) {
        LOGE("getTimeSec failed");
        return;
    }

    std::tm cur = {};
    if (!TimeUtils::localTime(currentTime, cur)) {
        LOGE("localtimeSafe failed, timestamp=%lld", static_cast<long long>(currentTime));
        return;
    }

    if (year > 0) {
        cur.tm_year = year - 1900;
    }

    if (month > 0) {
        if (month > 12) {
            LOGE("Invalid month=%d", month);
            return;
        }
        cur.tm_mon = month - 1;
    }

    if (day > 0) {
        cur.tm_mday = day;
    }

    if (hour >= 0) {
        if (hour > 23) {
            LOGE("Invalid hour=%d", hour);
            return;
        }
        cur.tm_hour = hour;
    }

    if (minute >= 0) {
        if (minute > 59) {
            LOGE("Invalid minute=%d", minute);
            return;
        }
        cur.tm_min = minute;
    }

    if (second >= 0) {
        if (second > 59) {
            LOGE("Invalid second=%d", second);
            return;
        }
        cur.tm_sec = second;
    }

    const int finalYear = cur.tm_year + 1900;
    const int finalMonth = cur.tm_mon + 1;
    const int finalDay = cur.tm_mday;

    if (!TimeUtils::isValidDate(finalYear, finalMonth, finalDay)) {
        LOGE("Invalid date year=%d month=%d day=%d", finalYear, finalMonth, finalDay);
        return;
    }

    if (!TimeUtils::isValidClock(cur.tm_hour, cur.tm_min, cur.tm_sec)) {
        LOGE("Invalid time hour=%d minute=%d second=%d", cur.tm_hour, cur.tm_min, cur.tm_sec);
        return;
    }

    cur.tm_isdst = -1;

    const std::time_t newTime = std::mktime(&cur);
    if (newTime == static_cast<std::time_t>(-1)) {
        LOGE("mktime failed while setTime");
        return;
    }

    setTime(static_cast<int64_t>(newTime));
}

void SystemUtils::syncHWClock() {
#ifdef PRODUCT_SIGMA
    asyncSystem("hwclock --systohc");
#endif
}

int SystemUtils::getTwscTpVersion() {
#ifndef PRODUCT_X64
    return 0;
#else
    int fd = open("/dev/techwin_ioctl", O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    int ret = ioctl(fd, 0xff);
    LOG(INFO) << "getTwscTpVersion: " << ret << std::endl;
    close(fd);
    return ret;
#endif
}
