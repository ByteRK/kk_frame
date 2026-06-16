/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:52
 * @LastEditTime: 2026-06-16 23:47:25
 * @FilePath: /kk_frame/src/utils/file_utils.cc
 * @Description: 文件相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "file_utils.h"
#include "system_utils.h"
#include <cdlog.h>
#include <ghc/filesystem.hpp>
#include <fstream>
#include <unistd.h>

void FileUtils::sync() {
    SystemUtils::sync();
}

bool FileUtils::have(const std::string& filePath) {
    return access(filePath.c_str(), F_OK) == 0;
}

bool FileUtils::read(const std::string& filePath, std::string& content) {
    ghc::filesystem::path path(filePath);
    if (!ghc::filesystem::exists(path) || ghc::filesystem::is_directory(path)) {
        LOGE("File not found or is a directory: %s", filePath.c_str());
        return false;
    }
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open file: %s", filePath.c_str());
        return false;
    }
    std::string content_cache((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.fail() && !file.eof()) {
        LOGE("Error reading file: %s", filePath.c_str());
        return false;
    }
    content_cache.swap(content);
    return true;
}

bool FileUtils::write(const std::string& filePath, const std::string& content) {
    ghc::filesystem::path path(filePath);
    if (ghc::filesystem::exists(path)) {
        if (!ghc::filesystem::is_regular_file(path)) {
            LOGE("Path is not a regular file: %s", filePath.c_str());
            return false;
        }
    }
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOGE("Failed to create file: %s", filePath.c_str());
        return false;
    }
    file << content;
    if (file.fail()) {
        LOGE("Failed to write file: %s", filePath.c_str());
        file.close();
        return false;
    }
    file.close();
    if (file.fail()) {
        LOGE("Failed to close file after write: %s", filePath.c_str());
        return false;
    }
    sync();
    return true;
}

bool FileUtils::check(const std::string& filePath, size_t* size) {
    ghc::filesystem::path path(filePath);
    if (ghc::filesystem::exists(path) && ghc::filesystem::is_regular_file(path)) {
        if (size) (*size) = ghc::filesystem::file_size(path);
        return true;
    }
    LOGW("File not found or is a directory: %s", filePath.c_str());
    return false;
}

bool FileUtils::check(const std::vector<std::string>& fileList, std::function<bool(const std::string&, size_t)> callback) {
    size_t size{ 0 };
    for (const std::string& file : fileList) {
        if (file.empty())continue;
        size = 0;
        if (check(file, &size)) if (callback(file, size))return true;
    }
    return false;
}
