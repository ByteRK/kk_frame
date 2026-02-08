/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2026-02-08 11:54:34
 * @FilePath: /kk_frame/src/utils/project_utils.cc
 * @Description: 项目相关的一些操作函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "project_utils.h"
#include "time_utils.h"

#include "fonts_info.h"
#include "series_info.h"
#include "app_version.h"

#include <gui_features.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cdlog.h>

void ProjectUtils::env() {
#if defined(PRODUCT_X64) || defined(__VSCODE__)
    setenv("FONTCONFIG_PATH", FONTCONFIG_PATH, 1);
    setenv("SCREEN_SIZE", SCREEN_SIZE, 1);
#endif
}

void ProjectUtils::pInfo(const char* name) {
    char szTmp[128];
    int id = syscall(SYS_gettid);

    fprintf(stderr, "\033[1;35m\n");

    // 猫咪
    fprintf(stderr, "\033[1;35m             __                 \n\033[0m");
    fprintf(stderr, "\033[1;35m            ` \\\\              \n\033[0m");
    fprintf(stderr, "\033[1;35m    /\\=/\\-\"\"-.//            \n\033[0m");
    fprintf(stderr, "\033[1;35m   = 'Y' =  ,  \\               \n\033[0m");
    fprintf(stderr, "\033[1;35m    '-^-'  /(  /                \n\033[0m");
    fprintf(stderr, "\033[1;35m     /;_,) |\\ \\ \\            \n\033[0m");
    fprintf(stderr, "\033[1;35m    (_/ (_/ (_(_/    神兽在此   #\n\033[0m");
    fprintf(stderr, "\033[1;35m    \"\"  \"\"  \"\" \"     漏洞退让   #\n\033[0m");

    fprintf(stderr, "\033[1;35m#################################\n");
    fprintf(stderr, "\033[1;35m# %s_%s_\033[0;39m", CDROID_BASE_OS, CDROID_PRODUCT);
#ifdef DEBUG
    fprintf(stderr, "\033[1;31mDEBUG");
#else
    fprintf(stderr, "\033[1;32mREALSE");
#endif
    fprintf(stderr, "\033[1;35m  [%s]\033[0;39m\n", APP_ID);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", name);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", APP_VER_INFO);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", BUILD_DATE);
    fprintf(stderr, "\033[1;35m# Git:%s\033[m\n", GIT_VERSION);
    fprintf(stderr, "\033[1;35m# Cdroid:V%s_%s_%s\033[m\n", CDROID_VERSION, CDROID_COMMITID, std::to_string(CDROID_BUILD_NUMBER).c_str());
    fprintf(stderr, "\033[1;35m############ Ricken #############\n\n\033[0m");
}

void ProjectUtils::pKeyMap() {
    fprintf(stderr, "\033[1;30m################################### KeyBoardMap ################################### \033[0;37m\n");
    fprintf(stderr, "\033[1;30m# 6:电源  5:鲜蒸  4:嫩烤  3:湿烤  2:飓风 ┏━━━━━━┓ 8:魔方  9:香炸 10:炖焗 11:智能 12:辅助   \033[0;37m\n");
    fprintf(stderr, "\033[1;30m# 7:加水  -:一一  -:一一  -:一一  -:一一 ┗━━━━━━┛ -:一一  -:一一  -:一一  -:一一  -:一一   \033[0;37m\n");
    fprintf(stderr, "\033[1;30m################################################################################### \033[0;37m\n");
    fprintf(stderr, "\033[0;37m\n");
}

void ProjectUtils::getDebugServiceInfo(std::string& ip, int16_t& port) {
    ip = "127.0.0.1";
    port = 8999;

#if defined(PRODUCT_X64) || defined(__VSCODE__)
    char* username = getlogin();
    if (username == nullptr) {
        LOGE("getlogin failed");
        return;
    }
    std::string nameStr(username);
    if (nameStr == "ricken") {
        ip = "10.0.0.10";
        port = 1140;
    } else {
        LOGW("unknown user: %s", username);
    }
#endif
}

int ProjectUtils::getTwscTpVersion() {
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

void ProjectUtils::saveTime(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening time file." << std::endl;
        return;
    }
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    file << "Current Date and Time: " << asctime(timeinfo);
    file.close();
#ifndef PRODUCT_X64
    std::system("sync");
#endif
}

void ProjectUtils::loadTime(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening time file." << std::endl;
        return;
    }
    std::string line;
    std::getline(file, line);
    char* cstr = new char[line.length() + 1];
    std::strcpy(cstr, line.c_str());
    tm timeinfo;
    strptime(cstr, "Current Date and Time: %a %b %d %H:%M:%S %Y\n", &timeinfo);
    delete[] cstr;
    std::cout << "Date and Time read from file: " << asctime(&timeinfo);
    TimeUtils::setTime(mktime(&timeinfo));
    file.close();
}

