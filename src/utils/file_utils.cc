/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:52
 * @LastEditTime: 2025-12-26 14:37:18
 * @FilePath: /kk_frame/src/utils/file_utils.cc
 * @Description: 文件相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "file_utils.h"
#include <cdlog.h>
#include <ghc/filesystem.hpp>

void FileUtils::sync() {
#ifdef CDROID_X64
    LOGI("-------- sync --------");
#else
    ::sync();
#endif
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
        if (ghc::filesystem::is_regular_file(path)) {
            ghc::filesystem::remove(path);
        } else {
            LOGE("Path is a directory: %s", filePath.c_str());
            return false;
        }
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to create file: %s", filePath.c_str());
        return false;
    }
    file << content;
    file.close();
    sync();
    return true;
}

bool FileUtils::check(const std::string& filePath, size_t& size) {
    ghc::filesystem::path path(filePath);
    if (ghc::filesystem::exists(path) && ghc::filesystem::is_regular_file(path)) {
        size = ghc::filesystem::file_size(path);
        return true;
    }
    LOGE("File not found or is a directory: %s", filePath.c_str());
    return false;
}
