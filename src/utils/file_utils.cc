/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:06:52
 * @LastEditTime: 2026-06-23 11:48:54
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

#if ENABLED(FILE_SYSTEM)
#include <ghc/filesystem.hpp>
#include <fstream>
#include <system_error>
#endif

#include <unistd.h>

bool FileUtils::isSupported() {
#if ENABLED(FILE_SYSTEM)
    return true;
#else
    return false;
#endif
}

#if ENABLED(FILE_SYSTEM)

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

bool FileUtils::remove(const std::string& filePath) {
    ghc::filesystem::path path(filePath);
    std::error_code ec;

    if (!ghc::filesystem::exists(path, ec)) return !ec;
    if (ec) {
        LOGE("Failed to check file: %s, err=%d", filePath.c_str(), ec.value());
        return false;
    }

    if (!ghc::filesystem::is_regular_file(path, ec) || ec) {
        LOGE("Path is not a regular file: %s", filePath.c_str());
        return false;
    }

    if (!ghc::filesystem::remove(path, ec) || ec) {
        LOGE("Failed to remove file: %s, err=%d", filePath.c_str(), ec.value());
        return false;
    }

    return true;
}

bool FileUtils::remove(const std::string& filePath, const std::string& backupPath) {
    // 不要用 && 串联，短路会跳过备份文件
    const bool result1 = filePath.empty() || remove(filePath);
    const bool result2 = backupPath.empty() || remove(backupPath);
    sync();

    return result1 && result2;
}

bool FileUtils::isDir(const std::string& path) {
    ghc::filesystem::path fsPath(path);
    return ghc::filesystem::exists(fsPath) && ghc::filesystem::is_directory(fsPath);
}

bool FileUtils::mkdir(const std::string& path) {
    ghc::filesystem::path fsPath(path);
    if (ghc::filesystem::exists(fsPath)) {
        if (ghc::filesystem::is_directory(fsPath)) return true;
        LOGE("Path exists but is not a directory: %s", path.c_str());
        return false;
    }
    if (!ghc::filesystem::create_directories(fsPath)) {
        LOGE("Failed to create directory: %s", path.c_str());
        return false;
    }
    sync();
    return true;
}

bool FileUtils::clearDir(const std::string& path) {
    ghc::filesystem::path fsPath(path);
    if (!ghc::filesystem::exists(fsPath)) return true;
    if (!ghc::filesystem::is_directory(fsPath)) {
        LOGE("Path is not a directory: %s", path.c_str());
        return false;
    }
    for (auto& entry : ghc::filesystem::directory_iterator(fsPath)) {
        if (ghc::filesystem::remove_all(entry.path()) == 0) {
            LOGE("Failed to remove: %s", entry.path().c_str());
            return false;
        }
    }
    sync();
    return true;
}

#else // !ENABLED(FILE_SYSTEM)

// ============================================================================
// ENABLE_FILE_SYSTEM 关闭时的失效实现
// 所有接口统一返回 false / 空操作，保证上层调用方无需改动即可编译运行。
// ============================================================================
namespace {
    /// @brief 统一提示，便于排查“功能未生效”的原因。
    void warnDisabled(const char* func) {
        LOGW("FileUtils::%s is unavailable: ENABLE_FILE_SYSTEM is disabled", func);
    }
}

void FileUtils::sync() { }

bool FileUtils::have(const std::string&) { warnDisabled("have"); return false; }

bool FileUtils::read(const std::string&, std::string&) { warnDisabled("read"); return false; }

bool FileUtils::write(const std::string&, const std::string&) { warnDisabled("write"); return false; }

bool FileUtils::check(const std::string&, size_t*) { warnDisabled("check"); return false; }

bool FileUtils::check(const std::vector<std::string>&, std::function<bool(const std::string&, size_t)>) {
    warnDisabled("check");
    return false;
}

bool FileUtils::remove(const std::string&) { warnDisabled("remove"); return false; }

bool FileUtils::remove(const std::string&, const std::string&) { warnDisabled("remove"); return false; }

bool FileUtils::isDir(const std::string&) { warnDisabled("isDir"); return false; }

bool FileUtils::mkdir(const std::string&) { warnDisabled("mkdir"); return false; }

bool FileUtils::clearDir(const std::string&) { warnDisabled("clearDir"); return false; }

#endif // ENABLED(FILE_SYSTEM)
