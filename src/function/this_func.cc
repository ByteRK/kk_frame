/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2024-08-22 17:15:40
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

#include <cdinput.h>
#include <core/inputeventsource.h>

void printProjectInfo(const char* name) {
    char szTmp[128];
    int id = syscall(SYS_gettid);

    fprintf(stderr, "\033[5;35m\n");

    // cdroid
    fprintf(stderr, "\033[5;35m        ┏━┓┏┓╋╋╋┏┓┏┓       \n\033[0m");
    fprintf(stderr, "\033[5;35m        ┃┏╋┛┣┳┳━╋╋┛┃       \n\033[0m");
    fprintf(stderr, "\033[5;35m        ┃┗┫╋┃┏┫╋┃┃╋┃       \n\033[0m");
    fprintf(stderr, "\033[5;35m        ┗━┻━┻┛┗━┻┻━┛       \n\033[0m");


    fprintf(stderr, "\033[1;35m############################\n");
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
    fprintf(stderr, "\033[1;35m# %21s\033[0;39m\n", HV_FORMAT_VERSION_STRING(szTmp));
    fprintf(stderr, "\033[1;35m# %21s\033[0;39m\n", HV_FORMAT_VER_TIME_STRING());
    fprintf(stderr, "\033[1;35m# Git:%s\033[m\n", HV_FORMAT_GIT_HARD_STRING());
    fprintf(stderr, "\033[1;35m########## Ricken ##########\n\n\033[0m");
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

uint32_t uint8_t_to_uint32_t(uint8_t* data, bool swap) {
    uint32_t value = 0;
    if (swap) {
        value = data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
    } else {
        value = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    }
    return value;
}
