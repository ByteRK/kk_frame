/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-05-28 09:07:45
 * @LastEditTime: 2025-05-28 10:30:26
 * @FilePath: /kk_frame/src/function/json_func.cc
 * @Description: Json数据处理
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "json_func.h"
#include <cdlog.h>
#include <fstream>
#include <ghc/filesystem.hpp>
#include <sstream>

template<typename T>
T jsonToType(const Json::Value& value, const T& defaultValue) {
    if (value.isNull()) return defaultValue;
    return value.as<T>();
}

template<typename T>
T getJsonValue(const Json::Value& root, const std::string& key, const T& defaultValue) {
    if (!root.isMember(key)) return defaultValue;
    return jsonToType<T>(root[key], defaultValue);
}

template<>
int jsonToType<int>(const Json::Value& value, const int& defaultValue) {
    if (value.isNull()) {
        LOGD("value is null");
        return defaultValue;
    }
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
        LOGD("Invalid numeric string: %s", str);
    }
    return defaultValue;
}

template<>
std::string jsonToType<std::string>(const Json::Value& value, const std::string& defaultValue) {
    if (value.isNull() || !value.isString()) return defaultValue;
    return value.asString();
}

template<>
bool jsonToType<bool>(const Json::Value& value, const bool& defaultValue) {
    if (value.isNull()) return defaultValue;
    if (value.isBool()) return value.asBool();
    if (value.isInt()) return value.asInt() != 0;
    if (value.isString()) {
        const std::string str = value.asString();
        return (str == "true" || str == "1");
    }
    return defaultValue;
}

bool convertStringToJson(const std::string& str, Json::Value& root) {
#if 0 // 旧接口，但简单
    Json::Reader reader(Json::Features::strictMode());
    if (reader.parse(str, root)) {
        return true;
    }
    LOGE("Json format error!!!!");
    return false;
#else
    Json::CharReaderBuilder readerBuilder;
    std::istringstream iss(str);
    std::string parseErrors;
    if (Json::parseFromStream(readerBuilder, iss, &root, &parseErrors)) {
        return true;
    }
    LOGE("JSON parse error: %s", parseErrors.c_str());
    return false;
#endif
}

bool convertJsonToString(const Json::Value& root, std::string& str, const std::string& indentation) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = indentation;
    str = Json::writeString(writer, root);
    return !str.empty();
}

bool loadLocalJson(const std::string& filePath, Json::Value& root) {
    // 文件不存在或者是一个目录
    ghc::filesystem::path path(filePath);
    if (!ghc::filesystem::exists(path) || ghc::filesystem::is_directory(path)) {
        LOGE("File not found or is a directory: %s", filePath.c_str());
        return false;
    }

    // 打开文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open file: %s", filePath.c_str());
        return false;
    }

    // 读取文件内容
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.fail() && !file.eof()) {
        LOGE("Error reading file: %s", filePath.c_str());
        return false;
    }

    return convertStringToJson(content, root);
}

bool saveLocalJson(const std::string& filePath, const Json::Value& root, const std::string& indentation) {
    std::string jsonString;
    if (!convertJsonToString(root, jsonString, indentation)) {
        LOGE("Failed to convert JSON to string");
        return false;
    }

    ghc::filesystem::path path(filePath);
    if (ghc::filesystem::exists(path)) {
        if (ghc::filesystem::is_regular_file(path)) {
            // 如果是同名文件则删除文件
            ghc::filesystem::remove(path);
        } else {
            LOGE("Path is a directory: %s", filePath.c_str());
            return false;
        }
    }

#if 1 // 是否使用二进制写
    std::ofstream file(filePath, std::ios::binary);
#else
    std::ofstream file(filePath);
#endif
    // 检测文件是否打开成功
    if (!file.is_open()) {
        LOGE("Failed to create file: %s", filePath.c_str());
        return false;
    }

    file << jsonString;
    // file.flush();
    file.close();

#if 1
    sync();
#else
    // 获取文件描述符并刷新数据到磁盘
    int fileDescriptor = outputFile.rdbuf()->pubseekoff(0, outputFile.beg, outputFile.out);
    if (fileDescriptor != -1) {
        if (fsync(fileDescriptor) == -1) {
            LOG(ERROR) << "Failed to flush data to disk";
            return false;
        }
    } else {
        LOG(ERROR) << "Failed to get file descriptor";
        return false;
    }
#endif

    return true;
}