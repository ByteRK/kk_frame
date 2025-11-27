/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:39:39
 * @LastEditTime: 2025-11-27 11:43:12
 * @FilePath: /cy_frame/src/function/json_func.h
 * @Description: Json数据处理
 * @BugList: 
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */

#ifndef __json_func_h__
#define __json_func_h__

#include <json/json.h>
#include <string>

// 将Json::Value转换为指定类型 | use -> jsonToType<T>(const Json::Value&, const T&)
template<typename T>
T jsonToType(const Json::Value& value, const T& defaultValue) {
    if (value.isNull()) return defaultValue;
    return value.as<T>();
}

// 获取Json::Value中的值，如果不存在则返回默认值
template<typename T>
T getJsonValue(const Json::Value& root, const std::string& key, const T& defaultValue) {
    if (!root.isMember(key)) return defaultValue;
    return jsonToType<T>(root[key], defaultValue);
}

/// @brief 将字符串转换为Json::Value
/// @param str 
/// @param root 
/// @return 
bool convertStringToJson(const std::string& str, Json::Value& root);

/// @brief 将Json::Value转换为字符串
/// @param root 
/// @param str 
/// @param indentation 传空为紧凑风格
/// @return 
bool convertJsonToString(const Json::Value& root, std::string& str, const std::string& indentation = "    ");

/// @brief 从本地文件加载Json数据
/// @param filePath 
/// @param root 
/// @return 
bool loadLocalJson(const std::string& filePath, Json::Value& root);

/// @brief 将Json::Value保存到本地文件
/// @param filePath 
/// @param root 
/// @param indentation 传空为紧凑风格
/// @return 
bool saveLocalJson(const std::string& filePath, const Json::Value& root, const std::string& indentation = "    ");


/************************************ START 特殊类型转换函数[数值,字符串,布尔] ****************************************/

template<>
inline int jsonToType<int>(const Json::Value& value, const int& defaultValue) {
    if (value.isNull()) return defaultValue;
    if (value.isInt()) {
        return value.asInt();
    }
    if (value.isString()) {
        const char* str = value.asCString();
        char* end;
        long num = strtol(str, &end, 10); // 非异常方法
        // 检查是否整个字符串被成功转换
        if (end != str && *end == '\0') {
            return static_cast<int>(num);
        }
    }
    return defaultValue;
}

template<>
inline std::string jsonToType<std::string>(const Json::Value& value, const std::string& defaultValue) {
    if (value.isNull() || !value.isString()) return defaultValue;
    return value.asString();
}

template<>
inline bool jsonToType<bool>(const Json::Value& value, const bool& defaultValue) {
    if (value.isNull()) return defaultValue;
    if (value.isBool()) return value.asBool();
    if (value.isInt()) return value.asInt() != 0;
    if (value.isString()) {
        const std::string str = value.asString();
        return (str == "true" || str == "1");
    }
    return defaultValue;
}

/************************************  END  特殊类型转换函数[数值,字符串,布尔] ****************************************/

#endif // __json_func_h__