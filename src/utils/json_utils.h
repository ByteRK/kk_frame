/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:39:39
 * @LastEditTime: 2026-04-24 15:18:57
 * @FilePath: /kk_frame/src/utils/json_utils.h
 * @Description: Json相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __JSON_UTILS_H__
#define __JSON_UTILS_H__

#include <json/json.h>
#include <functional>
#include <string>

namespace JsonUtils {

    /// @brief 内容校验回调，返回 false 表示该文件不可用，将继续尝试下一个文件
    typedef std::function<bool(const Json::Value&)> Validator;

    /// @brief 将JSON转换为指定类型
    /// @tparam T 返回值类型
    /// @param value JSON值
    /// @param defaultValue 默认值
    /// @return 键值或默认值
    template<typename T>
    T to(const Json::Value& value, const T& defaultValue) {
        if (value.isNull()) return defaultValue;
        return value.as<T>();
    }

    template<>
    inline int to<int>(const Json::Value& value, const int& defaultValue) {
        if (value.isNull()) return defaultValue;
        if (value.isInt()) return value.asInt();
        if (value.isString()) {
            char* end;
            const char* str = value.asCString();
            // 非异常方法
            long num = strtol(str, &end, 10);
            // 检查是否整个字符串被成功转换
            if (end != str && *end == '\0') return static_cast<int>(num);
        }
        return defaultValue;
    }

    template<>
    inline std::string to<std::string>(const Json::Value& value, const std::string& defaultValue) {
        if (!value.isNull() && value.isString()) return value.asString();
        return defaultValue;
    }

    template<>
    inline bool to<bool>(const Json::Value& value, const bool& defaultValue) {
        if (value.isNull()) return defaultValue;
        if (value.isBool()) return value.asBool();
        if (value.isInt()) return value.asInt() != 0;
        if (value.isString()) {
            const std::string str = value.asString();
            return (str == "true" || str == "1");
        }
        return defaultValue;
    }

    /// @brief 安全获取JSON值
    /// @tparam T 返回值类型
    /// @param json JSON对象
    /// @param key 键名
    /// @param defaultValue 默认值
    /// @return 键值或默认值
    template<typename T>
    T get(const Json::Value& json, const std::string& key, const T& defaultValue = T()) {
        if (!json.isMember(key)) return defaultValue;
        return to<T>(json[key], defaultValue);
    }

    /// @brief 解析JSON字符串
    /// @param str JSON字符串
    /// @param root JSON对象
    /// @return 是否成功
    bool parse(const std::string& jsonStr, Json::Value& root);

    /// @brief 将JSON对象转换为字符串
    /// @param json JSON对象
    /// @param pretty 是否格式化输出
    /// @return JSON字符串
    std::string toString(const Json::Value& json, bool pretty = false);

    /// @brief 从文件加载JSON
    /// @param filePath 文件路径
    /// @param json JSON对象
    /// @return 是否成功
    bool load(const std::string& filePath, Json::Value& json);

    /// @brief 从主/备文件加载JSON，主文件优先
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径，可传空
    /// @param json 输出的JSON数据
    /// @param loadingPath 实际加载的文件路径，可传 nullptr
    /// @param validator 内容校验，可传 nullptr
    /// @return 是否成功
    /// @note 读取失败、解析失败或校验不通过都会继续尝试备份文件
    bool load(const std::string& filePath, const std::string& backupPath,
        Json::Value& json, std::string* loadingPath = nullptr,
        const Validator& validator = Validator());

    /// @brief 保存JSON到文件
    /// @param filePath 文件路径
    /// @param json JSON对象
    /// @param pretty 是否格式化
    /// @return 是否成功
    bool save(const std::string& filePath, const Json::Value& json, bool pretty = false);

    /// @brief 保存JSON到主/备文件
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径
    /// @param json JSON对象
    /// @param isBackup true 写入备份文件，false 写入主文件
    /// @return 是否成功
    bool save(const std::string& filePath, const std::string& backupPath,
        const Json::Value& json, bool isBackup = false);
    
} // JsonUtils

#endif // !__JSON_UTILS_H__