/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:26
 * @LastEditTime: 2026-02-10 17:18:30
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

void SystemUtils::reboot() {
    ProjectUtils::saveTime(std::string(LOCAL_DATA_DIR) + "/timeCache");
#ifndef CDROID_X64
    std::system("sync");
    std::system("reboot");
#else
    exit(0);
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
    StringUtils::trimRight(result);

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
        _exit(127); // 如果 execl 失败
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
    value = value % 101;
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
