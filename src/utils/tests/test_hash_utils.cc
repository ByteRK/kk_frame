/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-05 17:30:00
 * @LastEditTime: 2026-11-05 17:30:00
 * @FilePath: /kk_frame/src/utils/tests/test_hash_utils.cc
 * @Description: HashUtils 用例（SHA256 标准向量 / MD5 失败语义）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "hash_utils.h"
#include "library/library_config.h"

TEST_CASE(hash_sha256Vectors) {
    // 空串
    EXPECT_STREQ(HashUtils::sha256CheckStr(std::string("")),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // "abc"
    EXPECT_STREQ(HashUtils::sha256CheckStr(std::string("abc")),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    // 56 字节标准向量（跨 block 边界，需要补两块）
    const std::string block(
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    EXPECT_STREQ(HashUtils::sha256CheckStr(block),
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_CASE(hash_md5UnavailableSemantics) {
    char buffer[HashUtils::MD5_STR_LEN];
    for (size_t i = 0; i < HashUtils::MD5_STR_LEN; ++i) buffer[i] = 'X';

    char* result = HashUtils::md5Check("abc", 3, buffer);
#if PRJ_LIB_ENABLED(MD5)
    // 启用 MD5 时只校验格式：32 位小写十六进制
    EXPECT_TRUE(result == buffer);
    for (size_t i = 0; i < HashUtils::MD5_STR_LEN; ++i) {
        const char ch = buffer[i];
        EXPECT_TRUE((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'));
    }
#else
    // 未启用时必须明确返回 nullptr，且不能把未初始化内容留给调用方
    EXPECT_TRUE(result == nullptr);
    EXPECT_EQ(buffer[0], '\0');
#endif
}
