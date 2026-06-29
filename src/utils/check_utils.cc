/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-06-29 23:31:05
 * @FilePath: /kk_frame/src/utils/check_utils.cc
 * @Description: 校验相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "check_utils.h"
#include "library_config.h"

#if PRJ_LIB_ENABLED(MD5)
#include "md5.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <cdlog.h>
#endif

namespace {

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
