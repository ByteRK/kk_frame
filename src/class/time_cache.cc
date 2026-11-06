/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-05 10:54:34
 * @FilePath: /kk_frame/src/class/time_cache.cc
 * @Description: 时间缓存文件读写（用于重启前的软件时钟保值）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "time_cache.h"
#include "clock_utils.h"
#include "file_utils.h"
#include "string_utils.h"

#include <cdlog.h>

#include <cstdint>
#include <ctime>

namespace {

    /// @brief 缓存文件路径（由应用启动阶段注入）
    std::string sCachePath;

    /// @brief 检查文件系统能力
    /// @param func 调用方函数名（仅用于日志）
    /// @return true 可用
    /// @note ENABLE_FILE_SYSTEM 关闭时 FileUtils 全部接口退化为失效实现，
    ///       此处显式 LOGE，避免时间缓存静默失效难以排查
    bool checkFileSystem(const char* func) {
        if (FileUtils::isSupported()) return true;
        LOGE("TimeCache::%s is unavailable: ENABLE_FILE_SYSTEM is disabled", func);
        return false;
    }

} // namespace

void TimeCache::setPath(const std::string& path) {
    sCachePath = path;
}

const std::string& TimeCache::path() {
    return sCachePath;
}

bool TimeCache::isReady() {
    return !sCachePath.empty();
}

bool TimeCache::save() {
    if (sCachePath.empty()) {
        LOGE("TimeCache path is not set");
        return false;
    }
    if (!checkFileSystem("save")) return false;

    // 原子写入：掉电时不会留下残缺文件，写完自动同步落盘
    const std::string content = std::to_string(static_cast<long long>(std::time(nullptr)));
    if (!FileUtils::writeAtomic(sCachePath, content)) {
        LOGE("Write time cache failed: %s", sCachePath.c_str());
        return false;
    }
    return true;
}

bool TimeCache::read(time_t& timestamp) {
    if (sCachePath.empty()) {
        LOGE("TimeCache path is not set");
        return false;
    }
    if (!checkFileSystem("read")) return false;

    std::string content;
    if (!FileUtils::read(sCachePath, content)) {
        LOGE("Read time cache failed: %s", sCachePath.c_str());
        return false;
    }

    int64_t value = 0;
    if (!StringUtils::toInt64(content, value) || value <= 0) {
        LOGE("Invalid timestamp in time cache: %s", sCachePath.c_str());
        return false;
    }

    timestamp = static_cast<time_t>(value);
    LOGI("time cache loaded: %lld", static_cast<long long>(value));
    return true;
}

bool TimeCache::recover() {
    time_t timestamp = 0;
    if (!read(timestamp)) {
        return false;
    }
    // 无权限（非 root）或平台不支持设置时钟时 setTime 返回 false，此处如实上报
    return ClockUtils::setTime(static_cast<int64_t>(timestamp));
}

bool TimeCache::clear() {
    if (sCachePath.empty()) {
        LOGE("TimeCache path is not set");
        return false;
    }
    if (!checkFileSystem("clear")) return false;

    // FileUtils::remove 对"文件本来就不存在"同样返回成功
    return FileUtils::remove(sCachePath);
}
