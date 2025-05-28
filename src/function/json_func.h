/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:39:39
 * @LastEditTime: 2025-05-28 10:29:30
 * @FilePath: /kk_frame/src/function/json_func.h
 * @Description: Json数据处理
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
**/

#ifndef __json_func_h__
#define __json_func_h__

#include "json/json.h"
#include <string>

// 将Json::Value转换为指定类型
template<typename T>
T jsonToType(const Json::Value& value, const T& defaultValue);

// 获取Json::Value中的值，如果不存在则返回默认值
template<typename T>
T getJsonValue(const Json::Value& root, const std::string& key, const T& defaultValue);

// 特殊类型转换函数[数值,字符串,布尔]
template<>
int jsonToType<int>(const Json::Value& value, const int& defaultValue);
template<>
std::string jsonToType<std::string>(const Json::Value& value, const std::string& defaultValue);
template<>
bool jsonToType<bool>(const Json::Value& value, const bool& defaultValue);

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

#endif // __json_func_h__