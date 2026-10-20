/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-19 10:00:00
 * @LastEditTime: 2026-10-19 10:00:00
 * @FilePath: /kk_frame/src/class/json_file_store.cc
 * @Description: 主/备 JSON 数据文件读写
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "json_file_store.h"
#include "json_utils.h"
#include "file_utils.h"
#include <cdlog.h>

bool JsonFileStore::load(Json::Value& json, const std::string& filePath,
    const std::string& backupPath, std::string* loadingPath,
    const Validator& validator) {
    std::string path;

    const bool result = FileUtils::check(
        { filePath, backupPath },
        [&json, &path, &validator](const std::string& file, size_t size) {
        if (size == 0)return false;

        // 先解析到临时对象，避免主文件解析失败时污染 JSON
        Json::Value value;
        if (!JsonUtils::load(file, value))return false;

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

bool JsonFileStore::save(const Json::Value& json, const std::string& filePath,
    const std::string& backupPath, bool isBackup) {
    const std::string& path = isBackup ? backupPath : filePath;
    if (path.empty()) {
        LOGE("JsonFileStore: empty file path");
        return false;
    }

    return JsonUtils::save(path, json);
}

bool JsonFileStore::remove(const std::string& filePath, const std::string& backupPath) {
    bool result = true;

    if (!filePath.empty()) result = FileUtils::remove(filePath) && result;
    if (!backupPath.empty()) result = FileUtils::remove(backupPath) && result;
    FileUtils::sync();

    return result;
}
