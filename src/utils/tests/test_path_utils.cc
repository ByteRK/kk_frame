/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/test_path_utils.cc
 * @Description: PathUtils / UrlUtils 用例
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "path_utils.h"
#include "url_utils.h"

TEST_CASE(path_joinAndSplit) {
    EXPECT_STREQ(PathUtils::join("/a", "b"), "/a/b");
    EXPECT_STREQ(PathUtils::join("/a/", "b"), "/a/b");
    EXPECT_STREQ(PathUtils::join("/a", "/b"), "/b");
    EXPECT_STREQ(PathUtils::join("", "b"), "b");
    EXPECT_STREQ(PathUtils::join("/a", ""), "/a");

    EXPECT_STREQ(PathUtils::fileName("/a/b/c.txt"), "c.txt");
    EXPECT_STREQ(PathUtils::fileName("/a/b/"), "b");
    EXPECT_STREQ(PathUtils::baseName("/a/b/c.txt"), "c");
    EXPECT_STREQ(PathUtils::extension("/a/b/c.txt"), ".txt");
    EXPECT_STREQ(PathUtils::extension("/a/b/c"), "");
    EXPECT_STREQ(PathUtils::dirName("/a/b/c.txt"), "/a/b");
    EXPECT_STREQ(PathUtils::dirName("/a"), "/");
    EXPECT_STREQ(PathUtils::dirName("a"), "");
}

TEST_CASE(path_normalize) {
    EXPECT_STREQ(PathUtils::normalize("/a//b/./c"), "/a/b/c");
    EXPECT_STREQ(PathUtils::normalize("/a/b/../c"), "/a/c");
    EXPECT_STREQ(PathUtils::normalize("a/../../b"), "../b");
    EXPECT_STREQ(PathUtils::normalize("/../a"), "/a");
    EXPECT_STREQ(PathUtils::normalize("/"), "/");
}

TEST_CASE(path_isSubPath) {
    EXPECT_TRUE(PathUtils::isSubPath("/a", "/a/b"));
    EXPECT_TRUE(PathUtils::isSubPath("/a", "/a"));
    EXPECT_TRUE(PathUtils::isSubPath("/a/", "/a/b"));
    EXPECT_TRUE(PathUtils::isSubPath("/", "/a/b"));

    // 前缀相似但不在目录内
    EXPECT_FALSE(PathUtils::isSubPath("/a", "/ab"));
    EXPECT_FALSE(PathUtils::isSubPath("/a/b", "/a"));

    // 用 ".." 试图逃出目录
    EXPECT_FALSE(PathUtils::isSubPath("/a/b", "/a/b/../../etc"));

    EXPECT_FALSE(PathUtils::isSubPath("", "/a"));
}

TEST_CASE(url_parse) {
    const std::string url = "https://user:pass@example.com:8443/a/b/c.zip?x=1#frag";

    EXPECT_STREQ(UrlUtils::scheme(url), "https");
    EXPECT_TRUE(UrlUtils::hasScheme(url));
    EXPECT_TRUE(UrlUtils::isHttpUrl(url));
    EXPECT_STREQ(UrlUtils::host(url), "example.com");
    EXPECT_STREQ(UrlUtils::port(url), "8443");
    EXPECT_STREQ(UrlUtils::path(url), "/a/b/c.zip");
    EXPECT_STREQ(UrlUtils::query(url), "x=1");
    EXPECT_STREQ(UrlUtils::fragment(url), "frag");
    EXPECT_STREQ(UrlUtils::fileName(url), "c.zip");

    EXPECT_STREQ(UrlUtils::scheme("example.com/a"), "");
    EXPECT_FALSE(UrlUtils::hasScheme("example.com/a"));
    EXPECT_FALSE(UrlUtils::isHttpUrl("ftp://example.com"));
    EXPECT_STREQ(UrlUtils::port("http://example.com/a"), "");
}

TEST_CASE(url_percent) {
    EXPECT_STREQ(UrlUtils::percentDecode("a%20b"), "a b");
    EXPECT_STREQ(UrlUtils::percentEncode("a b"), "a%20b");
    // 非法序列原样保留
    EXPECT_STREQ(UrlUtils::percentDecode("a%zzb"), "a%zzb");
}

TEST_CASE(url_fileNameRejectsTraversal) {
    // %2F 解码后是路径分隔符，必须拒绝解码、退回原始名称
    EXPECT_STREQ(UrlUtils::fileName("http://a/b/..%2F..%2Fetc%2Fpasswd"), "..%2F..%2Fetc%2Fpasswd");
    // 正常百分号编码仍应解码
    EXPECT_STREQ(UrlUtils::fileName("http://a/b/%E4%B8%AD%E6%96%87.zip"), "中文.zip");
    // 关闭解码时原样返回
    EXPECT_STREQ(UrlUtils::fileName("http://a/b/a%20b.zip", false), "a%20b.zip");
    // URL 以 "/" 结尾时没有文件名（下载落盘依赖该约束）
    EXPECT_STREQ(UrlUtils::fileName("http://a/b/"), "");
    EXPECT_STREQ(UrlUtils::fileName("http://a"), "");
}

TEST_CASE(url_join) {
    EXPECT_STREQ(UrlUtils::join("http://a/b", "c"), "http://a/b/c");
    EXPECT_STREQ(UrlUtils::join("http://a/b/", "c"), "http://a/b/c");
    EXPECT_STREQ(UrlUtils::join("http://a/b", "/c"), "http://a/c");
    EXPECT_STREQ(UrlUtils::join("http://a/b", "http://x/y"), "http://x/y");
    EXPECT_STREQ(UrlUtils::join("http://a", ""), "http://a");
}
