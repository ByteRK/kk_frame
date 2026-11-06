/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-11-04 15:00:00
 * @FilePath: /kk_frame/src/utils/integration/hash_utils.cc
 * @Description: 摘要算法（MD5 / SHA256）相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "hash_utils.h"

#include "bytes_utils.h"
#include "scope_guard.h"
#include "library/library_config.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#if PRJ_LIB_ENABLED(MD5)
#include "md5.h"
#include <fcntl.h>
#include <unistd.h>
#else
#include <cdlog.h>
#endif

namespace {

    constexpr size_t SHA256_BLOCK_SIZE = 64;

    const uint32_t SHA256_K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t rotateRight(uint32_t value, uint8_t bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    void sha256Transform(const uint8_t block[SHA256_BLOCK_SIZE], uint32_t state[8]) {
        uint32_t w[64];
        for (uint8_t i = 0; i < 16; i++) {
            w[i] = BytesUtils::readBig32(block + i * 4);
        }
        for (uint8_t i = 16; i < 64; i++) {
            uint32_t s0 = rotateRight(w[i - 15], 7) ^ rotateRight(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotateRight(w[i - 2], 17) ^ rotateRight(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        for (uint8_t i = 0; i < 64; i++) {
            uint32_t s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + s1 + ch + SHA256_K[i] + w[i];
            uint32_t s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

#if PRJ_LIB_ENABLED(MD5)
    /// @brief 将 MD5 原始摘要写成小写十六进制串
    /// @param md5Value MD5 原始摘要
    /// @param md5Str 输出缓冲区（长度恰为 MD5_STR_LEN，不写结尾 '\0'）
    /// @note 两个 md5Check 重载共用，避免重复实现 digest -> hex
    void md5DigestToHex(const unsigned char md5Value[HashUtils::MD5_SIZE],
        char md5Str[HashUtils::MD5_STR_LEN]) {
        const std::string hex = BytesUtils::hexEncode(md5Value, HashUtils::MD5_SIZE);
        std::memcpy(md5Str, hex.data(), HashUtils::MD5_STR_LEN);
    }
#endif

} // namespace

char* HashUtils::md5Check(const void* data, size_t len, char md5Str[MD5_STR_LEN]) {
#if PRJ_LIB_ENABLED(MD5)
    MD5_CTX       md5;
    unsigned char md5Value[MD5_SIZE];
    MD5Init(&md5);
    MD5Update(&md5, (unsigned char*)data, len);
    MD5Final(&md5, md5Value);
    md5DigestToHex(md5Value, md5Str);
    return md5Str;
#else
    (void)data;
    (void)len;
    LOGW("MD5 library is not enabled, cannot perform md5Check");
    // 失败时清空缓冲区，避免调用方把未初始化内容当摘要使用
    std::memset(md5Str, 0, MD5_STR_LEN);
    return nullptr;
#endif
}

char* HashUtils::md5Check(const char* filePath, char md5Str[MD5_STR_LEN]) {
#if PRJ_LIB_ENABLED(MD5)
    static const int READ_DATA_SIZE = 1024;
    unsigned char data[READ_DATA_SIZE];
    unsigned char md5_value[MD5_SIZE];
    MD5_CTX       md5;
    const int fd = open(filePath, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        LOGE("md5Check: open failed: %s errno=%d", filePath, errno);
        std::memset(md5Str, 0, MD5_STR_LEN);
        return nullptr;
    }
    // 任何提前返回都会关闭 fd，无需逐条路径手写 close
    ScopeGuard closeFd([fd]() { close(fd); });

    MD5Init(&md5);
    for (;;) {
        const int ret = read(fd, data, READ_DATA_SIZE);
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOGE("md5Check: read failed: %s errno=%d", filePath, errno);
            std::memset(md5Str, 0, MD5_STR_LEN);
            return nullptr;
        }
        if (ret == 0) break;
        MD5Update(&md5, data, ret);
    }
    MD5Final(&md5, md5_value);
    md5DigestToHex(md5_value, md5Str);
    return md5Str;
#else
    (void)filePath;
    LOGW("MD5 library is not enabled, cannot perform md5Check");
    std::memset(md5Str, 0, MD5_STR_LEN);
    return nullptr;
#endif
}

uint8_t* HashUtils::sha256Check(const void* data, size_t len, uint8_t sha256Value[SHA256_SIZE]) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t state[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    size_t processed = 0;
    while (len - processed >= SHA256_BLOCK_SIZE) {
        sha256Transform(bytes + processed, state);
        processed += SHA256_BLOCK_SIZE;
    }

    uint8_t finalBlock[SHA256_BLOCK_SIZE * 2] = { 0 };
    const size_t remaining = len - processed;
    if (remaining > 0) {
        memcpy(finalBlock, bytes + processed, remaining);
    }
    finalBlock[remaining] = 0x80;

    const size_t finalSize = (remaining < 56) ? SHA256_BLOCK_SIZE : SHA256_BLOCK_SIZE * 2;
    BytesUtils::writeBig64(static_cast<uint64_t>(len) * 8, finalBlock + finalSize - 8);

    sha256Transform(finalBlock, state);
    if (finalSize > SHA256_BLOCK_SIZE) {
        sha256Transform(finalBlock + SHA256_BLOCK_SIZE, state);
    }

    for (uint8_t i = 0; i < 8; i++) {
        BytesUtils::writeBig32(state[i], sha256Value + i * 4);
    }
    return sha256Value;
}

std::string HashUtils::sha256CheckStr(const void* data, size_t len) {
    uint8_t shaValue[SHA256_SIZE];
    sha256Check(data, len, shaValue);
    return BytesUtils::hexEncode(shaValue, SHA256_SIZE);
}

std::string HashUtils::sha256CheckStr(const std::string& src) {
    return sha256CheckStr(src.data(), src.size());
}
