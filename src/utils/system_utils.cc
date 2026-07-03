/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:26
 * @LastEditTime: 2026-07-03 15:04:09
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

#define TIME_CACHE_FILE "timeCache"

void SystemUtils::reboot() {
    ProjectUtils::saveTime(std::string(LOCAL_DATA_DIR) + TIME_CACHE_FILE);
    sync();
#ifndef PRODUCT_X64
    std::system("reboot");
#else
    ::exit(0);
#endif
}

void SystemUtils::exit() {
    ProjectUtils::saveTime(std::string(LOCAL_DATA_DIR) + TIME_CACHE_FILE);
    sync();
#ifndef PRODUCT_X64
    std::system("exit");
#else
    ::exit(0);
#endif
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
        LOGI("Write %s -> %s", value.c_str(), path.c_str());
    }
    return success;
}

void SystemUtils::setBrightness(int value, bool swap) {
    value %= 101;
    if (swap) value = 100 - value;
#if defined(PRODUCT_SIGMA)
#define BRIGHTNESS_PWM_PATH "/sys/class/pwm/pwmchip0/" SYS_SCREEN_BEIGHTNESS_PWM
#define BRIGHTNESS_ENABLE_PATH BRIGHTNESS_PWM_PATH "/enable"
#define BRIGHTNESS_VALUE_PATH BRIGHTNESS_PWM_PATH "/duty_cycle"

    if (access(BRIGHTNESS_PWM_PATH, F_OK)) { return; }

    if (value) {
        if (value == 0) value = 1;
        sysfs(BRIGHTNESS_ENABLE_PATH, "1");
        sysfs(BRIGHTNESS_VALUE_PATH, std::to_string(value));
        LOGV("设置屏幕背光 %d", value);
    } else {
        sysfs(BRIGHTNESS_ENABLE_PATH, "0");
        LOGV("关闭屏幕背光");
    }

#undef BRIGHTNESS_VALUE_PATH
#undef BRIGHTNESS_ENABLE_PATH
#undef BRIGHTNESS_PWM_PATH
#elif defined(PRODUCT_RK3506)
    int val = 255 * value / 100;
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/backlight/backlight/brightness", val);
    system(cmd);
#else
    LOGI("设置屏幕背光 %d | %d", value, swap);
#endif
}

void SystemUtils::setVolume(int value) {
    value %= 101;
#if defined(PRODUCT_RK3506)
    char cmd[256];
    // amixer scontrols
    // amixer set 'DAC Digital' 155~255
    // amixer set 'Master' 0~100
    if (level > 0) {
        level = 155 + level;
        snprintf(cmd, sizeof(cmd), "amixer set 'DAC Digital' %d", level);
        system("amixer set 'Master' 100");
    } else {
        snprintf(cmd, sizeof(cmd), "amixer set 'DAC Digital' 0");
        system("amixer set 'Master' 0");
    }
    system(cmd);
#else
    LOGI("设置音量 %d", value);
#endif
}

void SystemUtils::recoverTime() {
    ProjectUtils::loadTime(std::string(LOCAL_DATA_DIR) + TIME_CACHE_FILE);
}

void SystemUtils::clearTimeCache() {
    std::string cacheFile = std::string(LOCAL_DATA_DIR) + TIME_CACHE_FILE;
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
