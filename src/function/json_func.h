/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-06-18 07:58:01
 * @FilePath: /hana_frame/src/function/json_func.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef __json_func_h__
#define __json_func_h__

#include "json/json.h"


/// @brief json读值
/// @tparam T 
/// @param json 
/// @param key 
/// @param defaultValue 
/// @return 
template<typename T>
T jsonSafeGet(const Json::Value& json, const std::string& key, T defaultValue) {
    return json.isMember(key) ? json[key].as<T>() : defaultValue;
}

/// @brief 处理数组
/// @tparam T 返回值的类型
/// @param json JSON 数组
/// @param index 数组索引
/// @param defaultValue 默认值
/// @return 获取到的值或默认值
template<typename T>
T jsonSafeGet(const Json::Value& json, size_t index, T defaultValue) {
    if (json.isArray() && json.isValidIndex(static_cast<Json::ArrayIndex>(index))) {
        return json[static_cast<Json::ArrayIndex>(index)].as<T>();
    }
    return defaultValue;
}

/// @brief 直接获取 JSON 单值
/// @tparam T 返回值的类型
/// @param json JSON 单值
/// @param defaultValue 默认值
/// @return 获取到的值或默认值
template<typename T>
T jsonSafeGet(const Json::Value& json, T defaultValue) {
    if (!json.isNull()) {
        return json.as<T>();
    }
    return defaultValue;
}
/// @brief string转json
/// @brief 完整结构
/// @param str 
/// @param root 
/// @return 
bool convertStringToJson(const std::string& str, Json::Value& root);

/// @brief json转string
/// @brief 完整结构
/// @param root 
/// @param str 
/// @return 
bool convertJsonToString(const Json::Value& root, std::string& str);

/// @brief 加载本地Json文件
/// @param filePath 
/// @param root 
/// @return 
bool loadLocalJson(const std::string& filePath, Json::Value& root);

/// @brief 保存Json至本地文件
/// @param filePath 
/// @param root 
/// @return 
bool saveLocalJson(const std::string& filePath, const Json::Value& root);


#endif // __json_func_h__