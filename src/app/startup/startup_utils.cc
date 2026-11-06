/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:47:17
 * @LastEditTime: 2026-08-14 16:12:08
 * @FilePath: /kk_frame/src/app/startup/startup_utils.cc
 * @Description: 应用启动阶段的环境准备与信息输出
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "startup_utils.h"
#include "env_utils.h"
#include "brand_utils.h"
#include "network_utils.h"
#include "time_cache.h"
#include "tick_mgr.h"

#include "fonts_info.h"
#include "config_info.h"
#include "series_info.h"
#include "app_version.h"

#include <gui_features.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cdlog.h>

void StartupUtils::env() {
#if defined(PRODUCT_X64) || defined(__VSCODE__)
    setenv("FONTCONFIG_PATH", FONTCONFIG_PATH, 1);
    setenv("SCREEN_SIZE", SCREEN_SIZE, 1);
#endif
}

void StartupUtils::pInfo(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) { // 是否仅显示版本信息
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("%s\n", APP_VER_INFO);
#if 0
            ::exit(0);
#else
            fflush(stdout);
            ::_exit(0);
#endif
        }
    }

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
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", argv[0]);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", APP_VER_INFO);
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", BUILD_DATE);
    fprintf(stderr, "\033[1;35m# Git:%s\033[m\n", GIT_VERSION);
    fprintf(stderr, "\033[1;35m# Cdroid:V%s_%s_%s\033[m\n", CDROID_VERSION, CDROID_COMMITID, std::to_string(CDROID_BUILD_NUMBER).c_str());
    fprintf(stderr, "\033[1;35m# KK Frame:V2.1.0\033[m\n");
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", BrandUtils::copyrightLine().c_str());
    fprintf(stderr, "\033[1;35m# %s\033[0;39m\n", BrandUtils::basedOnLine().c_str());
    fprintf(stderr, "\033[1;35m############ %s #############\n\n\033[0m", BrandUtils::owner().c_str());
}

void StartupUtils::pKeyMap() {
    /**
     * 在此处输出项目映射的按键消息~
    **/

    // fprintf(stderr, "\033[1;30m################################### KeyBoardMap ################################### \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m# 6:电源  5:鲜蒸  4:嫩烤  3:湿烤  2:飓风 ┏━━━━━━┓ 8:魔方  9:香炸 10:炖焗 11:智能 12:辅助   \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m# 7:加水  -:一一  -:一一  -:一一  -:一一 ┗━━━━━━┛ -:一一  -:一一  -:一一  -:一一  -:一一   \033[0;37m\n");
    // fprintf(stderr, "\033[1;30m################################################################################### \033[0;37m\n");
    // fprintf(stderr, "\033[0;37m\n");
}

void StartupUtils::createDir() {
    // 构建Data目录
    // 说明：这里刻意直接使用 stat/mkdir 而不用 FileUtils::mkdir，
    //       因为目录创建是启动必经路径，不能受 ENABLE_FILE_SYSTEM 开关影响。
    const char* path = APP_DATA_DIR;
    struct stat st = {};
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            LOGD("[PROJECT] Directory already exists: %s", path);
        } else {
            LOGW("[PROJECT] Path exists but is not a directory: %s", path);
        }
    } else if (mkdir(path, 0755) == 0) {
        LOGI("[PROJECT] Directory created: %s", path);
    } else {
        LOGE("[PROJECT] Failed to create directory: %s (%s)", path, strerror(errno));
    }

    // 时间缓存路径依赖数据目录，统一在启动阶段注入
    TimeCache::setPath(std::string(path) + "timeCache");
}

namespace {
    /// @brief 时间缓存周期保存所用的定时任务
    /// @note 故意不释放：进程退出阶段无需再注销定时任务，
    ///       用静态指针避免全局对象析构顺序与 TickMgr 冲突
    TickMgr::ITickVariable* timeCacheTick() {
        static TickMgr::ITickVariable* sTickVar = new TickMgr::ITickVariable();
        return sTickVar;
    }
} // namespace

bool StartupUtils::startTimeCache(int intervalMs, int delayMs) {
    if (!TimeCache::isReady()) {
        LOGE("[PROJECT] TimeCache path is not set, skip periodic save");
        return false;
    }
    if (intervalMs <= 0) {
        LOGE("[PROJECT] invalid TimeCache interval: %d", intervalMs);
        return false;
    }

    TickMgr::ITickVariable* tickVar = timeCacheTick();
    tickVar->setTick(intervalMs);
    tickVar->setCallBack([](int64_t /*nowMs*/) {
        if (!TimeCache::save()) LOGE("TimeCache save failed");
    });

    // TickMgr 未初始化时 startTick 会返回 false，此处不能静默
    if (!tickVar->startTick(delayMs)) {
        LOGE("[PROJECT] TimeCache tick register failed");
        return false;
    }
    return true;
}

bool StartupUtils::stopTimeCache() {
    TickMgr::ITickVariable* tickVar = timeCacheTick();
    if (!g_tick->hasTick(tickVar)) {
        LOGW("[PROJECT] TimeCache tick is not running, skip stop");
        return false;
    }

    tickVar->stopTick();
    LOGI("[PROJECT] TimeCache tick stopped");
    return true;
}

void StartupUtils::getDebugServiceInfo(std::string& ip, uint16_t& port) {
    ip = "127.0.0.1";
    port = 8999;

#if defined(PRODUCT_X64) || defined(__VSCODE__)
    std::string ipStr;
    int portInt = 0;
    if (EnvUtils::getString("DEBUG_SERVICE_IP", ipStr) && EnvUtils::getInt("DEBUG_SERVICE_PORT", portInt)) {
        // 端口必须落在合法范围内，避免强转 uint16_t 后回绕成随机端口
        if (NetworkUtils::isValidPort(static_cast<uint32_t>(portInt))) {
            ip = ipStr;
            port = static_cast<uint16_t>(portInt);
            LOGI("getDebugServiceInfo: %s:%d", ip.c_str(), port);
            return;
        }
        LOGE("invalid DEBUG_SERVICE_PORT: %d", portInt);
    } else {
        LOGW("NO SET DEBUG_SERVICE_IP OR DEBUG_SERVICE_PORT");
    }

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


