/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-02 18:27:34
 * @FilePath: /kk_frame/src/utils/url_utils.cc
 * @Description: URL 解析与拼接相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "url_utils.h"

#include "bytes_utils.h"
#include "path_utils.h"

#include <algorithm>
#include <cctype>

namespace {

    const size_t kNotFound = std::string::npos;

    /// @brief 取 authority（host[:port]）部分，不含 userinfo 之前的内容
    std::string authority(const std::string& url) {
        const size_t schemePos = url.find("://");
        const size_t begin = (schemePos == kNotFound) ? 0 : schemePos + 3;
        size_t end = url.find_first_of("/?#", begin);
        if (end == kNotFound) {
            end = url.size();
        }
        return url.substr(begin, end - begin);
    }

    /// @brief 去掉 authority 中的 userinfo 部分
    std::string stripUserInfo(const std::string& text) {
        const size_t atPos = text.find_last_of('@');
        return (atPos == kNotFound) ? text : text.substr(atPos + 1);
    }

    /// @brief 是否为 RFC3986 未保留字符
    bool isUnreserved(unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~';
    }

} // namespace

std::string UrlUtils::scheme(const std::string& url) {
    const size_t end = url.find_first_of(":/?#");
    if (end == kNotFound || url[end] != ':') {
        return std::string();
    }
    return url.substr(0, end);
}

bool UrlUtils::hasScheme(const std::string& url) {
    return !scheme(url).empty();
}

bool UrlUtils::isHttpUrl(const std::string& url) {
    std::string proto = scheme(url);
    std::transform(proto.begin(), proto.end(), proto.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return proto == "http" || proto == "https";
}

std::string UrlUtils::host(const std::string& url) {
    const std::string text = stripUserInfo(authority(url));

    /* IPv6 字面量：[::1]:8080 */
    if (!text.empty() && text.front() == '[') {
        const size_t closePos = text.find(']');
        if (closePos == kNotFound) {
            return text.substr(1);
        }
        return text.substr(1, closePos - 1);
    }

    const size_t colonPos = text.find(':');
    if (colonPos == kNotFound) {
        return text;
    }
    return text.substr(0, colonPos);
}

std::string UrlUtils::port(const std::string& url) {
    const std::string text = stripUserInfo(authority(url));
    size_t colonPos = text.find(':');

    /* IPv6 字面量的端口写在中括号之后 */
    if (!text.empty() && text.front() == '[') {
        const size_t closePos = text.find(']');
        if (closePos == kNotFound) {
            return std::string();
        }
        colonPos = text.find(':', closePos);
    }

    if (colonPos == kNotFound) {
        return std::string();
    }
    return text.substr(colonPos + 1);
}

std::string UrlUtils::path(const std::string& url) {
    const size_t schemePos = url.find("://");
    size_t begin = 0;
    if (schemePos != kNotFound) {
        const size_t authBegin = schemePos + 3;
        const size_t authEnd = url.find_first_of("/?#", authBegin);
        if (authEnd == kNotFound || url[authEnd] != '/') {
            return std::string();
        }
        begin = authEnd;
    }

    size_t end = url.find_first_of("?#", begin);
    if (end == kNotFound) {
        end = url.size();
    }
    return url.substr(begin, end - begin);
}

std::string UrlUtils::query(const std::string& url) {
    const size_t hashPos = url.find('#');
    const std::string head = (hashPos == kNotFound) ? url : url.substr(0, hashPos);

    const size_t queryPos = head.find('?');
    if (queryPos == kNotFound) {
        return std::string();
    }
    return head.substr(queryPos + 1);
}

std::string UrlUtils::fragment(const std::string& url) {
    const size_t hashPos = url.find('#');
    if (hashPos == kNotFound) {
        return std::string();
    }
    return url.substr(hashPos + 1);
}

std::string UrlUtils::fileName(const std::string& url, bool decode) {
    const std::string urlPath = path(url);
    if (urlPath.empty() || urlPath.back() == PathUtils::separator) {
        return std::string();
    }

    const size_t slashPos = urlPath.find_last_of(PathUtils::separator);
    const std::string name = (slashPos == kNotFound)
        ? urlPath
        : urlPath.substr(slashPos + 1);

    if (!decode) {
        return name;
    }

    // 百分号解码可能把 %2F 变成 '/'、%2E%2E 变成 ".."，
    // 直接当作文件名会把下载写到目标目录之外，此处拒绝并退回原始名称
    const std::string decoded = percentDecode(name);
    if (decoded.find_first_of("/\\") != kNotFound || decoded == "." || decoded == "..") {
        return name;
    }
    return decoded;
}

std::string UrlUtils::join(const std::string& base, const std::string& relative) {
    if (relative.empty()) {
        return base;
    }
    if (hasScheme(relative)) {
        return relative;
    }
    if (base.empty()) {
        return relative;
    }

    /* 相对路径以 "/" 开头：仅保留 base 的协议与主机部分 */
    if (relative.front() == PathUtils::separator) {
        const size_t schemePos = base.find("://");
        if (schemePos == kNotFound) {
            return relative;
        }
        const size_t authBegin = schemePos + 3;
        const size_t authEnd = base.find_first_of("/?#", authBegin);
        const size_t keepSize = (authEnd == kNotFound) ? base.size() : authEnd;
        return base.substr(0, keepSize) + relative;
    }

    if (base.back() == PathUtils::separator) {
        return base + relative;
    }
    return base + PathUtils::separator + relative;
}

std::string UrlUtils::percentDecode(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size()) {
            const int high = BytesUtils::hexValue(text[i + 1]);
            const int low = BytesUtils::hexValue(text[i + 2]);
            if (high >= 0 && low >= 0) {
                result.push_back(static_cast<char>((high << 4) | low));
                i += 2;
                continue;
            }
        }
        result.push_back(text[i]);
    }
    return result;
}

std::string UrlUtils::percentEncode(const std::string& text) {
    static const char* kHexDigits = "0123456789ABCDEF";

    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        if (isUnreserved(ch)) {
            result.push_back(static_cast<char>(ch));
        } else {
            result.push_back('%');
            result.push_back(kHexDigits[(ch >> 4) & 0x0F]);
            result.push_back(kHexDigits[ch & 0x0F]);
        }
    }
    return result;
}

