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

#include "file_utils.h"
#include "path_utils.h"
#include "string_utils.h"
#include "url_utils.h"

#include <algorithm>
#include <string>

namespace {

    /// @brief 默认缓存目录
    const char* kDefaultCacheDir = "./netImageCache/";

    /// @brief 规范化缓存目录
    /// @param path 文件夹路径
    /// @return 归一化并补全尾部分隔符后的目录路径
    static std::string normalizeDirPath(const std::string& path) {
        std::string out = path.empty() ? kDefaultCacheDir : path;
        std::replace(out.begin(), out.end(), '\\', PathUtils::separator);
        return PathUtils::ensureTrailingSeparator(PathUtils::normalize(out));
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

    if (!UrlUtils::isHttpUrl(res)) {
        ImageView::setImageResource(res);
        return;
    }

    const std::string fileName = UrlUtils::fileName(res);
    if (!isSupportImageType(fileName)) {
        ImageView::setImageResource(res);
        return;
    }

    const std::string localPath = PathUtils::join(mCachePath, fileName);
    if (FileUtils::isFile(localPath)) {
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
    if (!FileUtils::isDir(mCachePath)) FileUtils::mkdir(mCachePath);
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
            const std::string fileName = UrlUtils::fileName(event.url);
            if (!fileName.empty()) {
                localPath = PathUtils::join(mCachePath, fileName);
            }
        }
        mRequestId = 0;
        if (event.response.isSuccess() &&
            !localPath.empty() &&
            FileUtils::isFile(localPath)) {
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
    // PathUtils::extension 返回带 '.' 的后缀（如 ".png"），统一转小写后比较
    const std::string ext = StringUtils::lowerNew(PathUtils::extension(fileName));
    return ext == ".jpg" ||
        ext == ".jpeg" ||
        ext == ".png" ||
        ext == ".bmp" ||
        ext == ".gif" ||
        ext == ".webp";
}
