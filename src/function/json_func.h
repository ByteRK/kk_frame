/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:39:39
 * @LastEditTime: 2024-06-06 02:57:18
 * @FilePath: /cy_frame/src/common/json_func.h
 * @Description: Json数据处理
 * @BugList: 
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */

#ifndef __json_func_h__
#define __json_func_h__

#include "json/json.h"

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