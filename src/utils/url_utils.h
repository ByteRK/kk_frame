/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-02 12:00:00
 * @FilePath: /kk_frame/src/utils/url_utils.h
 * @Description: URL 解析与拼接相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __URL_UTILS_H__
#define __URL_UTILS_H__

#include <string>

/// @brief URL 解析与拼接相关的一些函数
namespace UrlUtils {

    /// @brief 取 URL 的协议名（scheme）
    /// @param url 目标 URL
    /// @return 协议名（如 "http"）；无协议返回空字符串
    std::string scheme(const std::string& url);

    /// @brief 判断 URL 是否带有完整协议头（如 "http://"）
    bool hasScheme(const std::string& url);

    /// @brief 取主机名
    /// @param url 目标 URL
    /// @return 主机名（已去掉 userinfo、端口与 IPv6 方括号）；无法解析时返回空字符串
    std::string host(const std::string& url);

    /// @brief 取端口号
    /// @param url 目标 URL
    /// @return 端口字符串；未显式指定时返回空字符串
    std::string port(const std::string& url);

    /// @brief 取路径部分
    /// @param url 目标 URL
    /// @return 路径（不含协议头、查询串与片段）；无路径时返回空字符串
    std::string path(const std::string& url);

    /// @brief 取查询串
    /// @param url 目标 URL
    /// @return "?" 之后的内容（不含 "?" 与 "#"）；无查询串时返回空字符串
    std::string query(const std::string& url);

    /// @brief 取片段
    /// @param url 目标 URL
    /// @return "#" 之后的内容（不含 "#"）；无片段时返回空字符串
    std::string fragment(const std::string& url);

    /// @brief 从 URL 提取文件名
    /// @param url 目标 URL
    /// @param decode 是否对结果做百分号解码
    /// @return 文件名；URL 无文件名（如以 "/" 结尾）时返回空字符串
    /// @note 例："http://a/b/c.zip?x=1" -> "c.zip"
    std::string fileName(const std::string& url, bool decode = true);

    /// @brief 拼接 URL 与相对路径
    /// @param base 基础地址（可带或不带结尾 "/"）
    /// @param relative 相对路径或完整 URL
    /// @return 拼接结果；relative 为完整 URL 时直接返回 relative
    std::string join(const std::string& base, const std::string& relative);

    /// @brief 百分号解码
    /// @param text 待解码字符串
    /// @return 解码结果；非法序列原样保留
    std::string percentDecode(const std::string& text);

    /// @brief 百分号编码
    /// @param text 待编码字符串
    /// @return 编码结果；仅保留 RFC3986 未保留字符（字母、数字、-_.~）
    std::string percentEncode(const std::string& text);

    /// @brief 解析下载落盘路径
    /// @param url 下载地址
    /// @param downloadPath 目标路径，可以是目录也可以是具体文件
    /// @return downloadPath 为已存在的目录时自动追加 URL 文件名；
    ///         downloadPath 为文件路径时原样返回；
    ///         目录但无法从 URL 得到文件名时返回空字符串
    std::string resolveDownloadPath(const std::string& url, const std::string& downloadPath);

} // UrlUtils

#endif // !__URL_UTILS_H__
