/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-25 17:43:13
 * @LastEditTime: 2026-02-25 17:49:15
 * @FilePath: /kk_frame/src/utils/env_utils.cc
 * @Description: 环境变量相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "env_utils.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>
#include <algorithm>

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

        errno = 0;
        char* end = NULL;
        long result = std::strtol(value, &end, 10);

        if (errno != 0 || end == value || *end != '\0')
            return false;

        if (result < INT_MIN || result > INT_MAX)
            return false;

        out = static_cast<int>(result);
        return true;
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

        errno = 0;
        char* end = NULL;
        long long result = std::strtoll(value, &end, 10);

        if (errno != 0 || end == value || *end != '\0')
            return false;

        out = result;
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

        errno = 0;
        char* end = NULL;
        double result = std::strtod(value, &end);

        if (errno != 0 || end == value || *end != '\0')
            return false;

        out = result;
        return true;
    }

    double getDoubleOr(const char* name, double defaultValue) {
        double value;
        if (getDouble(name, value))
            return value;
        return defaultValue;
    }

    // ===================== bool =====================

    static std::string ToLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    bool getBool(const char* name, bool& out) {
        const char* value = std::getenv(name);
        if (!value)
            return false;

        std::string s = ToLower(value);

        if (s == "1" || s == "true" || s == "yes" || s == "on") {
            out = true;
            return true;
        }

        if (s == "0" || s == "false" || s == "no" || s == "off") {
            out = false;
            return true;
        }

        return false;
    }

    bool getBoolOr(const char* name, bool defaultValue) {
        bool value;
        if (getBool(name, value))
            return value;
        return defaultValue;
    }

} // namespace EnvUtils