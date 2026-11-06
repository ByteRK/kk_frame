/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 12:00:00
 * @LastEditTime: 2026-11-05 19:45:00
 * @FilePath: /kk_frame/src/utils/platform/sysnode_utils.cc
 * @Description: 系统虚拟文件节点（/sys、/proc）读写
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "sysnode_utils.h"
#include "string_utils.h"

#include <cdlog.h>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <utility>

bool SysNodeUtils::write(const std::string& path, const std::string& value) {
    FILE* fp = fopen(path.c_str(), "w");
    if (fp == nullptr) {
        LOGE("Failed to open %s", path.c_str());
        return false;
    }
    const bool success = (fprintf(fp, "%s", value.c_str()) > 0);
    fclose(fp);

    if (!success) {
        LOGE("Write failed: %s -> %s", value.c_str(), path.c_str());
    } else {
        LOGV("Write %s -> %s", value.c_str(), path.c_str());
    }
    return success;
}

int SysNodeUtils::readInt(const std::string& path, int def) {
    const int64_t value = readInt64(path, def);
    if (value > INT_MAX) {
        LOGW("Read [%lld] overflow int <- %s", static_cast<long long>(value), path.c_str());
        return INT_MAX;
    }
    if (value < INT_MIN) {
        LOGW("Read [%lld] overflow int <- %s", static_cast<long long>(value), path.c_str());
        return INT_MIN;
    }
    return static_cast<int>(value);
}

int64_t SysNodeUtils::readInt64(const std::string& path, int64_t def) {
    std::string text;
    if (!SysNodeUtils::readTextFile(path, text)) return def;

    int64_t value = 0;
    if (!StringUtils::toInt64(text, value)) {
        LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
        return def;
    }

    LOGV("Read [%lld] <- %s", static_cast<long long>(value), path.c_str());
    return value;
}

double SysNodeUtils::readDouble(const std::string& path, double def) {
    std::string text;
    if (!SysNodeUtils::readTextFile(path, text)) return def;

    double value = 0.0;
    if (!StringUtils::toDouble(text, value)) {
        LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
        return def;
    }

    LOGV("Read [%f] <- %s", value, path.c_str());
    return value;
}

std::string SysNodeUtils::readString(const std::string& path, const std::string& def) {
    std::string text;
    if (!SysNodeUtils::readTextFile(path, text) || text.empty()) return def;

    LOGV("Read [%s] <- %s", text.c_str(), path.c_str());
    return text;
}

bool SysNodeUtils::readBool(const std::string& path, bool def) {
    std::string text;
    if (!SysNodeUtils::readTextFile(path, text) || text.empty()) return def;

    bool value = false;
    if (StringUtils::toBool(text, value)) {
        LOGV("Read [%s] <- %s", value ? "true" : "false", path.c_str());
        return value;
    }

    LOGE("Read failed: %s [%s]", path.c_str(), text.c_str());
    return def;
}

bool SysNodeUtils::readTextFile(const std::string& path, std::string& content) {
    FILE* fp = fopen(path.c_str(), "r");
    if (fp == nullptr) {
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

    // 去除首尾空白（含换行符），便于调用方按行/按值解析
    content = StringUtils::trim(std::move(content));
    return true;
}
