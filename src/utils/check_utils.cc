/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-07-09 16:25:37
 * @FilePath: /kk_frame/src/utils/check_utils.cc
 * @Description: 校验相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "check_utils.h"
#include "encoding_utils.h"
#include "library_config.h"
#include <cstdio>
#include <cstring>

#if PRJ_LIB_ENABLED(MD5)
#include "md5.h"
#include <unistd.h>
#include <fcntl.h>
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
            w[i] = EncodingUtils::readBig32(block + i * 4);
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
            uint32_t s1    = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            uint32_t ch    = (e & f) ^ (~e & g);
            uint32_t temp1 = h + s1 + ch + SHA256_K[i] + w[i];
            uint32_t s0    = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            uint32_t maj   = (a & b) ^ (a & c) ^ (b & c);
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

    uint16_t crc16Normal(const uint8_t* data, size_t length, uint16_t initialValue) {
        uint16_t crc = initialValue;
        for (size_t i = 0; i < length; i++) {
            crc ^= static_cast<uint16_t>(data[i]) << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
            }
        }
        return crc;
    }

    uint16_t crc16Reflected(const uint8_t* data, size_t length,
        uint16_t initialValue, uint16_t polynomial) {
        uint16_t crc = initialValue;
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                crc = (crc & 0x0001) ? ((crc >> 1) ^ polynomial) : (crc >> 1);
            }
        }
        return crc;
    }

} // namespace

char* CheckUtils::md5Check(const void* data, size_t len, char md5Str[MD5_STR_LEN]) {
#if PRJ_LIB_ENABLED(MD5)
    int           i;
    MD5_CTX       md5;
    unsigned char md5Value[16];
    MD5Init(&md5);
    MD5Update(&md5, (unsigned char *)data, len);
    MD5Final(&md5, md5Value);
    for (i = 0; i < 16; i++) {
        snprintf(md5Str + i * 2, 2 + 1, "%02x", md5Value[i]);
    }
#else
    LOGW("MD5 library is not enabled, cannot perform md5Check");
#endif
    return md5Str;
}

char * CheckUtils::md5Check(const char * filePath, char md5Str[MD5_STR_LEN]) {
    static const int READ_DATA_SIZE = 1024;
#if PRJ_LIB_ENABLED(MD5)
    int           i;
    int           fd;
    int           ret;
    unsigned char data[READ_DATA_SIZE];
    unsigned char md5_value[MD5_SIZE];
    MD5_CTX       md5;
    fd = open(filePath, O_RDONLY);
    if (-1 == fd) {
        //perror("open");
        return md5Str;
    }
    MD5Init(&md5);
    while (1) {
        ret = read(fd, data, READ_DATA_SIZE);
        if (-1 == ret) {
            perror("read");
            close(fd);
            return md5Str;
        }
        MD5Update(&md5, data, ret);
        if (ret < READ_DATA_SIZE) { break; }
    }
    close(fd);
    MD5Final(&md5, md5_value);
    for (i = 0; i < MD5_SIZE; i++) {
        snprintf(md5Str + i * 2, 2 + 1, "%02x", md5_value[i]);
    }
#else
    LOGW("MD5 library is not enabled, cannot perform md5Check");
#endif
    return md5Str;
}

uint8_t* CheckUtils::sha256Check(const void* data, size_t len, uint8_t sha256Value[SHA256_SIZE]) {
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
    size_t  remaining                         = len - processed;
    if (remaining > 0) {
        memcpy(finalBlock, bytes + processed, remaining);
    }
    finalBlock[remaining] = 0x80;

    size_t finalSize = (remaining < 56) ? SHA256_BLOCK_SIZE : SHA256_BLOCK_SIZE * 2;
    EncodingUtils::writeBig64(static_cast<uint64_t>(len) * 8, finalBlock + finalSize - 8);

    sha256Transform(finalBlock, state);
    if (finalSize > SHA256_BLOCK_SIZE) {
        sha256Transform(finalBlock + SHA256_BLOCK_SIZE, state);
    }

    for (uint8_t i = 0; i < 8; i++) {
        EncodingUtils::writeBig32(state[i], sha256Value + i * 4);
    }
    return sha256Value;
}

std::string CheckUtils::sha256CheckStr(const void* data, size_t len) {
    static const char hex[] = "0123456789abcdef";
    uint8_t sha_hex[SHA256_SIZE];
    char    sha_str[SHA256_STR_LEN + 1] = { 0 };
    sha256Check(data, len, sha_hex);
    for (uint8_t i = 0; i < SHA256_SIZE; i++) {
        sha_str[i * 2]     = hex[sha_hex[i] >> 4];
        sha_str[i * 2 + 1] = hex[sha_hex[i] & 0x0F];
    }
    return std::string(sha_str);
}

std::string CheckUtils::sha256CheckStr(const std::string& src) {
    return sha256CheckStr(src.data(), src.size());
}

uint8_t CheckUtils::xorCheck(const uint8_t* data, uint64_t length) {
    uint8_t result = 0;
    for (uint64_t i = 0; i < length; i++) {
        result ^= data[i];
    }
    return result;
}

uint8_t CheckUtils::crc8MAXIM(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x01) ? ((crc >> 1) ^ 0x8C) : (crc >> 1);
        }
    }
    return crc;
}

uint8_t CheckUtils::crc8SMBUS(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16CCITT(const uint8_t* data, size_t length) {
    return crc16Normal(data, length, 0xFFFF);
}

uint16_t CheckUtils::crc16XMODEM(const uint8_t* data, size_t length) {
    return crc16Normal(data, length, 0x0000);
}

uint16_t CheckUtils::crc16KERMIT(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0x0000, 0x8408);
}

uint16_t CheckUtils::crc16MODBUS(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0xFFFF, 0xA001);
}

uint16_t CheckUtils::crc16ANSI(const uint8_t* data, size_t length) {
    return crc16Reflected(data, length, 0x0000, 0xA001);
}

uint16_t CheckUtils::crc16IBM(const uint8_t* data, size_t length) {
    return crc16ANSI(data, length);
}

uint32_t CheckUtils::crc32IEEE(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x00000001u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
