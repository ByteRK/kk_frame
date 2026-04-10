/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-09 22:40:19
 * @LastEditTime: 2026-04-10 23:38:43
 * @FilePath: /kk_frame/src/widgets/net_image_view.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "net_image_view.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

namespace {

    /// @brief 检查文件夹
    /// @param path 文件夹路径
    /// @return true/false
    static std::string normalizeDirPath(const std::string& path) {
        std::string out = path.empty() ? "./netImageCache/" : path;
        std::replace(out.begin(), out.end(), '\\', '/');
        if (out.empty()) out = "./";
        if (out.back() != '/') out.push_back('/');
        return out;
    }

    /// @brief 判断是否是http链接
    /// @param url http链接
    /// @return true/false
    static bool isHttpUrl(const std::string& url) {
        return url.compare(0, 7, "http://") == 0 ||
            url.compare(0, 8, "https://") == 0;
    }

    /// @brief 判断是否文件夹
    /// @param path 文件夹路径
    /// @return true/false
    static bool isDirectory(const std::string& path) {
        struct stat st;
        return (!path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    }

    /// @brief 判断是否是常规文件
    /// @param path 文件路径
    /// @return true/false
    static bool isRegularFile(const std::string& path) {
        struct stat st;
        return (!path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
    }

    /// @brief 遍历创建文件夹
    /// @param path 文件夹路径
    /// @return true/false
    static bool createDirRecursive(const std::string& path) {
        std::string normalized = path;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        while (!normalized.empty() && normalized.back() == '/') {
            normalized.pop_back();
        }

        if (normalized.empty()) return false;
        if (isDirectory(normalized)) return true;

        std::string current;
        size_t start = 0;

        if (normalized[0] == '/') {
            current = "/";
            start = 1;
        }

        while (start <= normalized.size()) {
            const size_t end = normalized.find('/', start);
            const std::string part = normalized.substr(
                start,
                (end == std::string::npos) ? (normalized.size() - start) : (end - start));

            if (!part.empty()) {
                if (!current.empty() && current != "/") current += "/";
                current += part;

                if (part != "." && part != ".." && !isDirectory(current)) {
                    if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
                        return false;
                    }
                }
            }

            if (end == std::string::npos) break;
            start = end + 1;
        }

        return isDirectory(normalized);
    }

    /// @brief 拼接路径
    /// @param dir 
    /// @param fileName 
    /// @return 
    static std::string joinPath(const std::string& dir, const std::string& fileName) {
        if (dir.empty()) return fileName;
        if (dir.back() == '/' || dir.back() == '\\') return dir + fileName;
        return dir + "/" + fileName;
    }

    /// @brief 从url中提取文件名
    /// @param url url
    /// @return 文件名
    static std::string extractFileNameFromUrl(const std::string& url) {
        if (url.empty()) return std::string();

        std::string cleanUrl = url;
        const size_t queryPos = cleanUrl.find_first_of("?#");
        if (queryPos != std::string::npos) {
            cleanUrl.erase(queryPos);
        }

        const size_t slashPos = cleanUrl.find_last_of('/');
        if (slashPos == std::string::npos) return cleanUrl;
        if (slashPos + 1 >= cleanUrl.size()) return std::string();

        return cleanUrl.substr(slashPos + 1);
    }

    /// @brief 获取文件类型
    /// @param fileName 文件名
    /// @return 文件类型
    static std::string getFileExt(const std::string& fileName) {
        const size_t dotPos = fileName.find_last_of('.');
        if (dotPos == std::string::npos || dotPos + 1 >= fileName.size()) {
            return std::string();
        }

        std::string ext = fileName.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        return ext;
    }

} // namespace


DECLARE_WIDGET(NetImageView);

NetImageView::NetImageView(int w, int h) : ImageView(w, h) {
    init();
}

NetImageView::NetImageView(Context* ctx, const AttributeSet& attrs) : ImageView(ctx, attrs) {
    init();
}

NetImageView::~NetImageView() {
    cancelDownload(false);
}

/// @brief 设置缓存路径
/// @param path 路径
void NetImageView::setCachePath(const std::string& path) {
    if (path.empty()) return;
    mCachePath = normalizeDirPath(path);
    init();
}

/// @brief 设置图片资源
/// @param res 资源
void NetImageView::setImageResource(const std::string& res) {
    cancelDownload(false);

    if (!isHttpUrl(res)) {
        ImageView::setImageResource(res);
        return;
    }

    const std::string fileName = extractFileNameFromUrl(res);
    if (!isSupportImageType(fileName)) {
        ImageView::setImageResource(res);
        return;
    }

    const std::string localPath = joinPath(mCachePath, fileName);
    if (isRegularFile(localPath)) {
        LOGI("[NetImageView] Use Cache Image: %s", localPath.c_str());
        ImageView::setImageResource(localPath);
        return;
    }

    ImageView::setImageResource(mDefaultImageRes);

    HttpManager::Request request = HttpManager::Request::CreateDownload(
        res,
        localPath,
        this);

    request.followRedirects = true;
    request.storeResponseBody = false;
    request.emitProgress = false;

    mRequestId = g_http->submit(request);

    LOGI("[NetImageView] Request Net Image: %s", res.c_str());
}

/// @brief 设置默认图片资源
/// @param res 资源
void NetImageView::setDefaultImageResource(const std::string& res) {
    mDefaultImageRes = res;
}

/// @brief 是否正在下载
/// @return 是否正在下载
bool NetImageView::isDownloading() const {
    return mRequestId != 0;
}

/// @brief 取消下载
/// @param reset 是否重置图片资源
void NetImageView::cancelDownload(bool reset) {
    if (!isDownloading()) return;
    g_http->cancel(mRequestId);
    mRequestId = 0;
    if (reset) ImageView::setImageResource(mDefaultImageRes);
}

/// @brief 初始化
void NetImageView::init() {
    mCachePath = normalizeDirPath(mCachePath);
    createDirRecursive(mCachePath);
}

/// @brief 处理http事件
/// @param event 事件
void NetImageView::onHttpEvent(const HttpManager::Event& event) {
    if (event.requestId != mRequestId) {
        return;
    }

    switch (event.type) {
    case HttpManager::EventType::STARTED: {
        // mRequestId = event.requestId;
    } break;
    case HttpManager::EventType::COMPLETED: {
        std::string localPath = event.response.outputFilePath;
        if (localPath.empty()) {
            const std::string fileName = extractFileNameFromUrl(event.url);
            if (!fileName.empty()) {
                localPath = joinPath(mCachePath, fileName);
            }
        }
        mRequestId = 0;
        if (event.response.isSuccess() &&
            !localPath.empty() &&
            isRegularFile(localPath)) {
            ImageView::setImageResource(localPath);
        } else {
            ImageView::setImageResource(mDefaultImageRes);
        }
    } break;
    case HttpManager::EventType::CANCELLED: {
        mRequestId = 0;
    } break;
    default: {
    } break;
    }
}

/// @brief 判断文件类型是否支持
/// @param fileName 文件名
/// @return 是否支持
bool NetImageView::isSupportImageType(const std::string& fileName) {
    const std::string ext = getFileExt(fileName);
    return ext == "jpg" ||
        ext == "jpeg" ||
        ext == "png" ||
        ext == "bmp" ||
        ext == "gif" ||
        ext == "webp";
}
