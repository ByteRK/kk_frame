/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-02 18:28:24
 * @FilePath: /kk_frame/src/utils/path_utils.cc
 * @Description: 路径字符串处理相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "path_utils.h"

#include <sys/stat.h>

#include <vector>

namespace {

    const size_t kNotFound = std::string::npos;

} // namespace

bool PathUtils::hasTrailingSeparator(const std::string& path) {
    return !path.empty() && path.back() == separator;
}

std::string PathUtils::ensureTrailingSeparator(const std::string& path) {
    if (path.empty() || path.back() == separator) {
        return path;
    }
    return path + separator;
}

std::string PathUtils::removeTrailingSeparator(const std::string& path) {
    if (path.size() <= 1) {
        return path;
    }
    std::string result = path;
    while (result.size() > 1 && result.back() == separator) {
        result.pop_back();
    }
    return result;
}

std::string PathUtils::join(const std::string& dir, const std::string& name) {
    if (name.empty()) {
        return dir;
    }
    if (isAbsolute(name)) {
        return name;
    }
    if (dir.empty()) {
        return name;
    }
    return ensureTrailingSeparator(dir) + name;
}

bool PathUtils::isAbsolute(const std::string& path) {
    return !path.empty() && path.front() == separator;
}

std::string PathUtils::fileName(const std::string& path) {
    if (path.empty()) {
        return std::string();
    }

    const std::string trimmed = removeTrailingSeparator(path);
    const size_t slashPos = trimmed.find_last_of(separator);
    if (slashPos == kNotFound) {
        return trimmed;
    }
    return trimmed.substr(slashPos + 1);
}

std::string PathUtils::baseName(const std::string& path) {
    const std::string name = fileName(path);
    const size_t dotPos = name.find_last_of('.');
    if (dotPos == kNotFound || dotPos == 0) {
        return name;
    }
    return name.substr(0, dotPos);
}

std::string PathUtils::extension(const std::string& path) {
    const std::string name = fileName(path);
    const size_t dotPos = name.find_last_of('.');
    if (dotPos == kNotFound || dotPos == 0 || dotPos + 1 >= name.size()) {
        return std::string();
    }
    return name.substr(dotPos);
}

std::string PathUtils::dirName(const std::string& path) {
    if (path.empty()) {
        return std::string();
    }

    const std::string trimmed = removeTrailingSeparator(path);
    const size_t slashPos = trimmed.find_last_of(separator);
    if (slashPos == kNotFound) {
        return std::string();
    }
    if (slashPos == 0) {
        return std::string(1, separator);
    }
    return trimmed.substr(0, slashPos);
}

std::string PathUtils::normalize(const std::string& path) {
    if (path.empty()) {
        return std::string();
    }

    const bool absolute = isAbsolute(path);
    const bool keepTrailing = hasTrailingSeparator(path);
    std::vector<std::string> segments;

    size_t begin = 0;
    while (begin <= path.size()) {
        size_t end = path.find(separator, begin);
        if (end == kNotFound) {
            end = path.size();
        }

        const std::string segment = path.substr(begin, end - begin);
        if (segment.empty() || segment == ".") {
            /* 空段（重复分隔符）与当前目录直接跳过 */
        } else if (segment == "..") {
            if (!segments.empty() && segments.back() != "..") {
                segments.pop_back();
            } else if (!absolute) {
                segments.push_back(segment);
            }
        } else {
            segments.push_back(segment);
        }

        begin = end + 1;
    }

    std::string result;
    if (absolute) {
        result.push_back(separator);
    }
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            result.push_back(separator);
        }
        result += segments[i];
    }

    if (result.empty()) {
        result = absolute ? std::string(1, separator) : std::string(".");
    } else if (keepTrailing && result.back() != separator) {
        result.push_back(separator);
    }
    return result;
}

bool PathUtils::exists(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0;
}

bool PathUtils::isDir(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode);
}

bool PathUtils::isFile(const std::string& path) {
    struct stat pathStat;
    return !path.empty() && stat(path.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode);
}
