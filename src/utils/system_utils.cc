/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:26
 * @LastEditTime: 2026-11-04 10:20:18
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
#include "path_utils.h"
#include "series_info.h"
#include "config_info.h"
#include <cdlog.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
            int max = SystemUtils::readSysfsInt(PathUtils::join(cfg.root, "max_brightness"), 0);
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

void SystemUtils::sync() {
#ifndef PRODUCT_X64
    std::system("sync");
#else
    LOGI("-------- sync --------");
#endif
}

void SystemUtils::exit() {
    ProjectUtils::saveTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
    sync();
    ::exit(0);
}

void SystemUtils::reboot() {
    ProjectUtils::saveTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
    sync();
#ifndef PRODUCT_X64
    std::system("reboot");
#else
    ::exit(0);
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

void SystemUtils::recoverTime() {
    ProjectUtils::loadTime(std::string(APP_DATA_DIR) + TIME_CACHE_FILE);
}

void SystemUtils::clearTimeCache() {
    std::string cacheFile = std::string(APP_DATA_DIR) + TIME_CACHE_FILE;
    system(std::string("rm -f ") + cacheFile);
    sync();
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
    int64_t value = readSysfsInt64(path, def);
    if (value > INT_MAX) {
        LOGW("Read [%lld] overflow int <- %s", (long long)value, path.c_str());
        return INT_MAX;
    }
    if (value < INT_MIN) {
        LOGW("Read [%lld] overflow int <- %s", (long long)value, path.c_str());
        return INT_MIN;
    }
    return static_cast<int>(value);
}

int64_t SystemUtils::readSysfsInt64(const std::string& path, int64_t def) {
    std::string text;
    if (!readTextFile(path, text)) return def;

    char*     end = nullptr;
    errno = 0;
    long long value = strtoll(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE) {
        LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
        return def;
    }

    LOGV("Read [%lld] <- %s", value, path.c_str());
    return static_cast<int64_t>(value);
}

double SystemUtils::readSysfsDouble(const std::string& path, double def) {
    std::string text;
    if (!readTextFile(path, text)) return def;

    char*  end = nullptr;
    errno = 0;
    double value = strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE) {
        LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
        return def;
    }

    LOGV("Read [%f] <- %s", value, path.c_str());
    return value;
}

std::string SystemUtils::readSysfsString(const std::string& path, const std::string& def) {
    std::string text;
    if (!readTextFile(path, text) || text.empty()) return def;

    LOGV("Read [%s] <- %s", text.c_str(), path.c_str());
    return text;
}

bool SystemUtils::readSysfsBool(const std::string& path, bool def) {
    std::string text;
    if (!readTextFile(path, text) || text.empty()) return def;

    static const char* const trueValues[] = { "1", "y", "yes", "true", "on", "enable", "enabled" };
    static const char* const falseValues[] = { "0", "n", "no", "false", "off", "disable", "disabled" };

    const std::string key = StringUtils::lowerNew(text);
    for (const char* item : trueValues) {
        if (key == item) {
            LOGV("Read [true] <- %s", path.c_str());
            return true;
        }
    }
    for (const char* item : falseValues) {
        if (key == item) {
            LOGV("Read [false] <- %s", path.c_str());
            return false;
        }
    }

    LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
    return def;
}

bool SystemUtils::readTextFile(const std::string& path, std::string& content) {
    FILE* fp = fopen(path.c_str(), "r");
    if (!fp) {
        LOGE("Failed to open %s", path.c_str());
        return false;
    }

    char   buffer[512];
    size_t len = 0;
    content.clear();
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        content.append(buffer, len);
    }
    const bool success = (ferror(fp) == 0);
    fclose(fp);

    if (!success) {
        LOGE("Read failed: %s", path.c_str());
        return false;
    }

    // 去除首尾空白（含换行符），便于直接按值解析
    content = StringUtils::trim(std::move(content));
    return true;
}

SystemUtils::DiskUsage SystemUtils::getDiskUsage(const std::string& path) {
    DiskUsage usage;
    struct statvfs stat = {};
    if (statvfs(path.c_str(), &stat) != 0) return usage;

    usage.valid = true;
    usage.total = static_cast<uint64_t>(stat.f_blocks) * stat.f_frsize;
    usage.available = static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    return usage;
}

int SystemUtils::createEventFd() {
    return eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

bool SystemUtils::notifyEventFd(int fd) {
    if (fd < 0) return false;

    const uint64_t value = 1;
    ssize_t result = 0;
    do {
        result = write(fd, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);

    // 计数器已满只说明通知已处于待处理状态，无需视为失败
    return result >= 0 || errno == EAGAIN || errno == EWOULDBLOCK;
}

bool SystemUtils::drainEventFd(int fd) {
    if (fd < 0) return false;

    uint64_t value = 0;
    while (true) {
        const ssize_t result = read(fd, &value, sizeof(value));
        if (result > 0) continue;
        if (result < 0 && errno == EINTR) continue;
        return result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK);
    }
}

bool SystemUtils::destroyEventFd(int& fd) {
    if (fd < 0) {
        fd = -1;
        return true;
    }

    // close 返回 EINTR 时 fd 状态不确定，按惯例不重试
    const int result = close(fd);
    fd = -1;
    if (result != 0) {
        LOGE("close eventfd failed. errno=%d", errno);
        return false;
    }
    return true;
}

std::string SystemUtils::errnoMessage(const char* prefix, int errorCode) {
    std::string message(prefix == nullptr ? "errno" : prefix);
    message += ", errno=";
    message += std::to_string(errorCode);
    message += ", ";
    message += std::strerror(errorCode);
    return message;
}
