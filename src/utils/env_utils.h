/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-25 17:43:13
 * @LastEditTime: 2026-02-25 17:49:19
 * @FilePath: /kk_frame/src/utils/env_utils.h
 * @Description: 环境变量相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ENV_UTILS_H__
#define __ENV_UTILS_H__

#include <string>
#include <type_traits>

namespace EnvUtils {

    // ===================== string =====================

    bool getString(const char* name, std::string& out);
    std::string getStringOr(const char* name, const std::string& defaultValue);

    // ===================== integer =====================

    bool getInt(const char* name, int& out);
    int getIntOr(const char* name, int defaultValue);

    bool getLongLong(const char* name, long long& out);
    long long getLongLongOr(const char* name, long long defaultValue);

    // ===================== double =====================

    bool getDouble(const char* name, double& out);
    double getDoubleOr(const char* name, double defaultValue);

    // ===================== bool =====================

    bool getBool(const char* name, bool& out);
    bool getBoolOr(const char* name, bool defaultValue);

    // ===================== enum =====================

    template<typename Enum>
    typename std::enable_if<std::is_enum<Enum>::value, bool>::type
        getEnum(const char* name, Enum& out) {
        int value = 0;
        if (!getInt(name, value))
            return false;

        out = static_cast<Enum>(value);
        return true;
    }

    template<typename Enum>
    typename std::enable_if<std::is_enum<Enum>::value, Enum>::type
        getEnumOr(const char* name, Enum defaultValue) {
        Enum value;
        if (getEnum(name, value))
            return value;
        return defaultValue;
    }

} // namespace EnvUtils

#endif // !__ENV_UTILS_H__
