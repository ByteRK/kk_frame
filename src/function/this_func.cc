/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2024-12-16 10:16:11
 * @FilePath: /kk_frame/src/function/this_func.cc
 * @Description: 此项目的一些功能函数
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#include "this_func.h"
#include "comm_func.h"

#include "hv_version.h"
#include "hv_series_conf.h"

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

void printProjectInfo(const char* name) {
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
    // 版本
    fprintf(stderr, "\033[1;35m#   ");
#ifdef CDROID_SIGMA
    fprintf(stderr, "\033[1;33m SIGMA ");
#else
    fprintf(stderr, "\033[1;33m  X64  ");
#endif
#ifdef DEBUG
    fprintf(stderr, "\033[1;31m DEBUG\n");
#else
    fprintf(stderr, "\033[1;32m REALSE\n");
#endif
    // 信息
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", name);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", HV_FORMAT_VERSION_STRING(szTmp));
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", HV_FORMAT_VER_TIME_STRING());
    fprintf(stderr, "\033[1;35m# Git:%s\033[m\n", HV_FORMAT_GIT_HARD_STRING());
    fprintf(stderr, "\033[1;35m############ Ricken #############\n\n\033[0m");
}

void printKeyMap() {
    // fprintf(stderr, "\033[1;30m############################ KeyBoardMap ############################\033[0;37m\n");
    // fprintf(stderr, "\033[1;30m# Q:电源  W:左炉  E:风速  R:增加 ┏━━━━━━┓ U:温时  I:蒸蒸  O:气炸  P:预约   \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m# A:照明  S:右炉  D:自动  F:减少 ┃      ┃ H:启停  J:烤烤  K:附加  L:炉灯   \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m# Z:清洗  -:一一  -:一一  -:一一 ┗━━━━━━┛ -:一一  -:一一  -:一一  M:童锁   \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m#####################################################################\033[0;37m\n");
    // fprintf(stderr, "\033[0;37m\n");
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

    struct timeval tv;
    tv.tv_sec = mktime(&timeinfo);
    tv.tv_usec = 0;
#ifndef DEBUG
    settimeofday(&tv, NULL);
#endif

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
