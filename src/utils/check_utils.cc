/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:11
 * @LastEditTime: 2026-04-24 15:07:32
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
#endif

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

uint16_t CheckUtils::crc16CCITT(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0x0000;   //初始值
    uint16_t i, j;
    for (j = 0; j < length; j++) {
        crc ^= data[j];
        for (i = 0; i < 8; i++) {
            if ((crc & 0x0001) > 0) {
                crc = (crc >> 1) ^ 0x8408;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16MODBUS(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xffff;   //初始值
    uint16_t i, j;
    for (j = 0; j < length; j++) {
        crc ^= data[j];
        for (i = 0; i < 8; i++) {
            if ((crc & 0x0001) > 0) {
                crc = (crc >> 1) ^ 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16ANSI(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0x0000;  // 初始值
    uint8_t i, j;
    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x8408;  // 多项式 0x1021 的位反射						
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

uint16_t CheckUtils::crc16IBM(const uint8_t* data, uint16_t length) {
    return crc16ANSI(data, length);
}
