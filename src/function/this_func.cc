/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-06-18 08:10:52
 * @FilePath: /hana_frame/src/function/this_func.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */


#include "this_func.h"
#include "comm_func.h"

#include "app_version.h"
#include "series_config.h"

#include <sys/syscall.h>   // for SYS_xxx definitions
#include <unistd.h>        // for syscall()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <codecvt>
#include <cstring>
#include <ctime>
#include <sys/time.h>

#include <cdlog.h>
#include <cdinput.h>
#include <core/inputeventsource.h>
#include <random> 

void printProjectInfo(const char* name) {
    char szTmp[128];
    int id = syscall(SYS_gettid);

    // print_cartoon_cat();
    // 信息
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", APP_ID);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", name);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", APP_VER_INFO);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", BUILD_DATE);
    fprintf(stderr, "\033[1;35m# Git:%s\033[m\n", GIT_VERSION);
    fprintf(stderr, "\033[1;35m############ HANAKAMi #############\n\n\033[0m");
}

void printKeyMap() {
    fprintf(stderr, "\033[1;30m################################### KeyBoardMap ################################### \033[0;37m\n");
    fprintf(stderr, "\033[1;30m# 6:电源  5:鲜蒸  4:嫩烤  3:湿烤  2:飓风 ┏━━━━━━┓ 8:魔方  9:香炸 10:炖焗 11:智能 12:辅助   \033[0;37m\n");
    fprintf(stderr, "\033[1;30m# 7:加水  -:一一  -:一一  -:一一  -:一一 ┗━━━━━━┛ -:一一  -:一一  -:一一  -:一一  -:一一   \033[0;37m\n");
    fprintf(stderr, "\033[1;30m################################################################################### \033[0;37m\n");
    fprintf(stderr, "\033[0;37m\n");
}

void analogInput(int code, int value) {
    INPUTEVENT i = { 0 };
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    i.tv_sec = ts.tv_sec;
    i.tv_usec = ts.tv_nsec / 1000;
#ifdef EV_KEY
    i.type = EV_KEY;
#else
    i.type = 0x01;
#endif
    i.code = code;
    i.value = value;
    i.device = INJECTDEV_KEY;
    InputInjectEvents(&i, 1, 1);
}

void refreshScreenSaver() {
#if 1
    SENDKEY(cdroid::KEY_EISU, 2);
#else
    cdroid::InputEventSource::getInstance().closeScreenSaver();
#endif
}

void writeCurrentDateTimeToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening time file." << std::endl;
        return;
    }

    time_t now = time(0);
    tm* timeinfo = localtime(&now);

    file << "Current Date and Time: " << asctime(timeinfo);

    file.close();

#ifndef CDROID_X64
    std::system("sync");
#endif
}

void setDateTimeFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening time file." << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);

    // Extracting date and time from the line
    char* cstr = new char[line.length() + 1];
    std::strcpy(cstr, line.c_str());

    tm timeinfo;
    strptime(cstr, "Current Date and Time: %a %b %d %H:%M:%S %Y\n", &timeinfo);

    delete[] cstr;

    std::cout << "Date and Time read from file: " << asctime(&timeinfo);

    timeSet(mktime(&timeinfo));

    file.close();
}

void defaultReboot() {
#ifndef CDROID_X64
    writeCurrentDateTimeToFile("/appconfigs/nowTimeCache");
    std::system("sync");
    std::system("reboot");
#else
    writeCurrentDateTimeToFile("./nowTimeCache");
    exit(0);
#endif
}

void print_cartoon_cat() {
    // 颜色定义
    const char* COLOR_FACE   = "\033[38;5;230m";  // 米白色脸部
    const char* COLOR_EARS   = "\033[38;5;179m";  // 浅棕色耳朵
    const char* COLOR_EYES   = "\033[38;5;97m";   // 紫色眼睛
    const char* COLOR_NOSE   = "\033[38;5;219m";  // 粉色鼻子
    const char* COLOR_WHISKERS= "\033[38;5;250m"; // 灰色胡须
    const char* COLOR_TEXT   = "\033[38;5;225m";  // 浅粉色文字
    const char* RESET        = "\033[0m";

    // 动态生成不同表情（随机切换）
    const char* eyes[] = { "◕ ‿ ◕", "◕ ᴥ ◕", "◕ ω ◕", "◕ ﻌ ◕" };
    const char* mouth[] = { " ▽ ", " ︶ ", " ω ", " × " };
    
    fprintf(stderr, "%s", COLOR_EARS);
    fprintf(stderr, "    /\\_____/\\    \n");
    fprintf(stderr, "   /         \\   \n");
    
    fprintf(stderr, "%s", COLOR_FACE);
    fprintf(stderr, "  (   %s%s%-5s%s  )  \n", COLOR_EYES, eyes[1], "", COLOR_FACE);
    fprintf(stderr, "   \\   %s%s%-3s%s   /   \n", COLOR_NOSE, mouth[0], "", COLOR_FACE);
    
    fprintf(stderr, "%s", COLOR_WHISKERS);
    fprintf(stderr, "   /  |   |  \\  \n");
    
    fprintf(stderr, "%s", COLOR_FACE);
    fprintf(stderr, "  (   %s%s%s   )  \n", COLOR_TEXT, "≧◡≦", COLOR_FACE);
    
    fprintf(stderr, "%s", COLOR_EARS);
    fprintf(stderr, "   \\_________/   \n");
    
    fprintf(stderr, "%s", COLOR_TEXT);
    fprintf(stderr, "    %sฅ^•ﻌ•^ฅ%s\n", COLOR_NOSE, RESET);
    fprintf(stderr, "   %s喵喵出击！%s\n\n", COLOR_TEXT, RESET);
}
