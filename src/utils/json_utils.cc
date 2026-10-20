/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:39:39
 * @LastEditTime: 2025-12-26 14:31:22
 * @FilePath: /kk_frame/src/utils/json_utils.cc
 * @Description: Json相关的一些操作函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "json_utils.h"
#include "file_utils.h"
#include <cdlog.h>
#include <fstream>
#include <sstream>

bool JsonUtils::parse(const std::string& jsonStr, Json::Value& root) {
    Json::CharReaderBuilder readerBuilder;
    std::istringstream iss(jsonStr);
    std::string parseErrors;
    if (Json::parseFromStream(readerBuilder, iss, &root, &parseErrors)) {
        return true;
    }
    LOGE("JSON parse error: %s", parseErrors.c_str());
    return false;
}

std::string JsonUtils::toString(const Json::Value& json, bool pretty) {
    Json::StreamWriterBuilder writer;
    if (pretty) writer["indentation"] = "    "; // 设置缩进为4个空格
    else writer["indentation"] = "";            // 不缩进
    return Json::writeString(writer, json);
}

bool JsonUtils::load(const std::string& filePath, Json::Value& json) {
    std::string content;
    return FileUtils::read(filePath, content) && parse(content, json);
}

bool JsonUtils::load(const std::string& filePath, const std::string& backupPath,
    Json::Value& json, std::string* loadingPath, const Validator& validator) {
    std::string path;

    const bool result = FileUtils::check(
        { filePath, backupPath },
        [&json, &path, &validator](const std::string& file, size_t size) {
        if (size == 0)return false;

        // 先解析到临时对象，避免主文件解析失败时污染 JSON
        Json::Value value;
        if (!load(file, value))return false;

        // 结构校验不通过时继续尝试备份文件
        if (validator && !validator(value)) {
            LOGE("invalid content, skip file: %s", file.c_str());
            return false;
        }

        json = value;
        path = file;
        return true;
    });

    if (loadingPath) *loadingPath = path;
    return result;
}

bool JsonUtils::save(const std::string& filePath, const Json::Value& json, bool pretty) {
    std::string jsonString = toString(json, pretty);
    if (jsonString.empty()) {
        LOGE("Failed to convert JSON to string");
        return false;
    }
    return FileUtils::write(filePath, jsonString);
}

bool JsonUtils::save(const std::string& filePath, const std::string& backupPath,
    const Json::Value& json, bool isBackup) {
    const std::string& path = isBackup ? backupPath : filePath;
    if (path.empty()) {
        LOGE("JsonUtils: empty file path");
        return false;
    }

    return save(path, json);
}