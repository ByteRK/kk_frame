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

bool JsonUtils::save(const std::string& filePath, const Json::Value& json, bool pretty) {
    std::string jsonString = toString(json, pretty);
    if (jsonString.empty()) {
        LOGE("Failed to convert JSON to string");
        return false;
    }
    return FileUtils::write(filePath, jsonString);
}