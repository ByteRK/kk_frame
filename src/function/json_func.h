/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-28 15:21:29
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
    if (json.isArray() && json.isValidIndex(index)) {
        return json[index].as<T>();
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

/// @brief 获取Int数据
/// @param root 
/// @param key 
/// @param defaultValue 
/// @return 
int getJsonInt(const Json::Value& root, const std::string& key, int defaultValue = 0);

/// @brief 获取Strig数据
/// @brief 使用返回值时建议使用std::move()
/// @param root 
/// @param key 
/// @param defaultValue 
/// @return 
std::string getJsonString(const Json::Value& root, const std::string& key, const std::string& defaultValue = "");

/// @brief 获取Bool数据
/// @param root 
/// @param key 
/// @param defaultValue 
/// @return 
bool getJsonBool(const Json::Value& root, const std::string& key, bool defaultValue = false);

/// @brief Json Value转Int
/// @param value 
/// @param defaultValue 
/// @return 
int jsonToInt(const Json::Value& value, int defaultValue);

/// @brief Json Value转String
/// @param value 
/// @param defaultValue 
/// @return 
std::string jsonToString(const Json::Value& value, const std::string& defaultValue);

/// @brief Json Value转Bool
/// @param value 
/// @param defaultValue 
/// @return 
bool jsonToBool(const Json::Value& value, bool defaultValue);

#endif // __json_func_h__