/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-25 17:43:13
 * @LastEditTime: 2026-11-05 11:28:07
 * @FilePath: /kk_frame/src/utils/platform/env_utils.cc
 * @Description: 环境变量相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "env_utils.h"
#include "string_utils.h"

#include <cstdlib>
#include <cstdint>

namespace EnvUtils {

    // ===================== string =====================

    bool getString(const char* name, std::string& out) {
        if (!name)
            return false;

        const char* value = std::getenv(name);
        if (!value)
            return false;

        out = value;
        return true;
    }

    std::string getStringOr(const char* name, const std::string& defaultValue) {
        std::string value;
        if (getString(name, value))
            return value;
        return defaultValue;
    }

    // ===================== int =====================

    bool getInt(const char* name, int& out) {
        const char* value = std::getenv(name);
        if (!value)
            return false;

        return StringUtils::toInt(value, out);
    }

    int getIntOr(const char* name, int defaultValue) {
        int value;
        if (getInt(name, value))
            return value;
        return defaultValue;
    }

    // ===================== long long =====================

    bool getLongLong(const char* name, long long& out) {
        const char* value = std::getenv(name);
        if (!value)
            return false;

        int64_t parsed = 0;
        if (!StringUtils::toInt64(value, parsed))
            return false;

        out = static_cast<long long>(parsed);
        return true;
    }

    long long getLongLongOr(const char* name, long long defaultValue) {
        long long value;
        if (getLongLong(name, value))
            return value;
        return defaultValue;
    }

    // ===================== double =====================

    bool getDouble(const char* name, double& out) {
        const char* value = std::getenv(name);
        if (!value)
            return false;

        return StringUtils::toDouble(value, out);
    }

    double getDoubleOr(const char* name, double defaultValue) {
        double value;
        if (getDouble(name, value))
            return value;
        return defaultValue;
    }

    // ===================== bool =====================

    bool getBool(const char* name, bool& out) {
        const char* value = std::getenv(name);
        if (!value)
            return false;
        return StringUtils::toBool(value, out);
    }

    bool getBoolOr(const char* name, bool defaultValue) {
        bool value;
        if (getBool(name, value))
            return value;
        return defaultValue;
    }

} // namespace EnvUtils