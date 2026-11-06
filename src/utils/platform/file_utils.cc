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
#include "process_utils.h"
#include <cdlog.h>

#include <sys/stat.h>
#include <sys/statvfs.h>

#if ENABLED(FILE_SYSTEM)
#include <ghc/filesystem.hpp>
#include <path_utils.h>
#include <fcntl.h>
#include <fstream>
#include <atomic>
#include <cerrno>
#include <system_error>
#include <unistd.h>
#endif

/* ============================================================================
 * 路径查询（只读，不受 ENABLE_FILE_SYSTEM 影响）
** ========================================================================== */

bool FileUtils::exists(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0;
}

bool FileUtils::isDir(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode);
}

bool FileUtils::isFile(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode);
}

FileUtils::DiskUsage FileUtils::getUsage(const std::string& path) {
    DiskUsage usage;
    struct statvfs stat = {};
    if (statvfs(path.c_str(), &stat) != 0) return usage;

    usage.valid = true;
    usage.total = static_cast<uint64_t>(stat.f_blocks) * stat.f_frsize;
    usage.available = static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    return usage;
}

/* ============================================================================
 * 文件读写与目录操作（受 ENABLE_FILE_SYSTEM 控制）
** ========================================================================== */

bool FileUtils::isSupported() {
#if ENABLED(FILE_SYSTEM)
    return true;
#else
    return false;
#endif
}

#if ENABLED(FILE_SYSTEM)

namespace {

    /// @brief 将数据完整写入 fd（处理部分写与 EINTR）
    bool writeAll(int fd, const char* data, size_t length) {
        size_t written = 0;
        while (written < length) {
            const ssize_t result = ::write(fd, data + written, length - written);
            if (result < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            written += static_cast<size_t>(result);
        }
        return true;
    }

    /// @brief 生成与目标同目录的唯一临时文件名
    /// @param filePath 目标文件路径
    /// @return "<目标>.tmp.<pid>.<序号>"
    /// @note 只带 pid 无法区分同进程的多线程，必须再叠加进程内自增序号，
    ///       否则两个线程写同一路径会共用临时文件并互相截断
    std::string makeTempPath(const std::string& filePath) {
        static std::atomic<uint64_t> seq{ 0 };
        return filePath + ".tmp."
            + std::to_string(static_cast<long long>(::getpid())) + "."
            + std::to_string(static_cast<unsigned long long>(
                seq.fetch_add(1, std::memory_order_relaxed) + 1));
    }

    /// @brief 同步目录项（保证 rename 本身落盘，掉电后先出现空文件的情况）
    /// @param dir 目标目录；为空（单段相对路径）时回退为当前目录
    void syncDir(const std::string& dir) {
        // 空目录名等价于当前目录，直接 return 会让 rename 结果缺少落盘保证
        const std::string target = dir.empty() ? std::string(".") : dir;

        const int fd = ::open(target.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (fd < 0) {
            LOGW("writeAtomic: open dir failed: %s errno=%d", target.c_str(), errno);
            return;
        }
        if (::fsync(fd) != 0) {
            LOGW("writeAtomic: fsync dir failed: %s errno=%d", target.c_str(), errno);
        }
        ::close(fd);
    }

} // namespace

void FileUtils::sync() {
    ProcessUtils::sync();
}

bool FileUtils::have(const std::string& filePath) {
    // 存在性判定只保留一套实现（FileUtils::exists），避免两处 stat/access 语义分叉
    return exists(filePath);
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

bool FileUtils::writeAtomic(const std::string& filePath, const std::string& content) {
    if (filePath.empty()) {
        LOGE("writeAtomic: empty path");
        return false;
    }

    // 临时文件必须与目标同目录，否则 rename 可能跨文件系统而失败
    // 名字带进程号与进程内序号，避免并发写同一路径时互相踩踏
    const std::string tempPath = makeTempPath(filePath);

    const int fd = ::open(tempPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        LOGE("writeAtomic: failed to create temp file: %s errno=%d", tempPath.c_str(), errno);
        return false;
    }

    // 先 fsync 数据再 rename：否则掉电后可能出现"新文件名 + 空/旧内容"
    const bool written = writeAll(fd, content.data(), content.size()) && ::fsync(fd) == 0;
    const int writeErrno = errno;
    ::close(fd);
    if (!written) {
        LOGE("writeAtomic: failed to write temp file: %s errno=%d", tempPath.c_str(), writeErrno);
        ::unlink(tempPath.c_str());
        return false;
    }

    // rename 是原子操作：读者要么看到旧文件，要么看到完整的新文件
    if (::rename(tempPath.c_str(), filePath.c_str()) != 0) {
        LOGE("writeAtomic: rename failed. src=%s dst=%s errno=%d",
            tempPath.c_str(), filePath.c_str(), errno);
        ::unlink(tempPath.c_str());
        return false;
    }

    // 同步目录项，确保重命名结果本身也落盘
    syncDir(PathUtils::dirName(filePath));
    return true;
}

bool FileUtils::append(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath.c_str(), std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        LOGE("Failed to open for append: %s", filePath.c_str());
        return false;
    }

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    const bool success = !file.fail();
    file.close();

    if (!success) {
        LOGE("Failed to append file: %s", filePath.c_str());
    }
    return success;
}

bool FileUtils::size(const std::string& filePath, size_t& size) {
    ghc::filesystem::path path(filePath);
    std::error_code ec;
    if (!ghc::filesystem::is_regular_file(path, ec) || ec) {
        return false;
    }

    size = static_cast<size_t>(ghc::filesystem::file_size(path, ec));
    return !ec;
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

bool FileUtils::check(const std::vector<std::string>& fileList, std::function<bool(const std::string&, size_t)> callback) {
    size_t size{ 0 };
    for (const std::string& file : fileList) {
        if (file.empty())continue;
        size = 0;
        if (check(file, &size)) if (callback(file, size))return true;
    }
    return false;
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

bool FileUtils::removeDir(const std::string& path) {
    ghc::filesystem::path fsPath(path);
    std::error_code ec;
    if (!ghc::filesystem::exists(fsPath, ec)) {
        return !ec;
    }

    ghc::filesystem::remove_all(fsPath, ec);
    if (ec) {
        LOGE("Failed to remove directory: %s, err=%d", path.c_str(), ec.value());
        return false;
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

bool FileUtils::writeAtomic(const std::string&, const std::string&) { warnDisabled("writeAtomic"); return false; }

bool FileUtils::append(const std::string&, const std::string&) { warnDisabled("append"); return false; }

bool FileUtils::size(const std::string&, size_t& size) { warnDisabled("size"); size = 0; return false; }

bool FileUtils::check(const std::string&, size_t*) { warnDisabled("check"); return false; }

bool FileUtils::remove(const std::string&) { warnDisabled("remove"); return false; }

bool FileUtils::remove(const std::string&, const std::string&) { warnDisabled("remove"); return false; }

bool FileUtils::check(const std::vector<std::string>&, std::function<bool(const std::string&, size_t)>) {
    warnDisabled("check");
    return false;
}

bool FileUtils::mkdir(const std::string&) { warnDisabled("mkdir"); return false; }

bool FileUtils::clearDir(const std::string&) { warnDisabled("clearDir"); return false; }

bool FileUtils::removeDir(const std::string&) { warnDisabled("removeDir"); return false; }

#endif // ENABLED(FILE_SYSTEM)
