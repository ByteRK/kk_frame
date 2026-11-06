/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-06 10:20:00
 * @FilePath: /kk_frame/src/utils/tests/test_check_utils.cc
 * @Description: CheckUtils 用例（带标准测试向量）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "check_utils.h"

namespace {

    /// @brief "123456789" 的 ASCII 字节（CRC 标准测试向量）
    const uint8_t kCheck[] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };
    const size_t kCheckLen = sizeof(kCheck) / sizeof(kCheck[0]);

} // namespace

TEST_CASE(checksum_sum) {
    EXPECT_EQ(CheckUtils::checkSum(kCheck, kCheckLen),
        static_cast<uint64_t>('1' * 9 + (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8)));

    const uint8_t data[] = { 0x01, 0x02, 0x03 };
    EXPECT_EQ(CheckUtils::checkSum(data, 3), static_cast<uint64_t>(6));
}

TEST_CASE(checksum_xor) {
    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x03 };
    EXPECT_EQ(CheckUtils::xorCheck(data, 4), 0x03);
    EXPECT_EQ(CheckUtils::xorCheck(data, 3), 0x00);
}

TEST_CASE(checksum_crcVectors) {
    // 以下为 check 值取自 CRC 目录（"123456789" 标准测试向量）
    EXPECT_EQ(CheckUtils::crc8MAXIM(kCheck, kCheckLen), 0xA1);
    EXPECT_EQ(CheckUtils::crc8SMBUS(kCheck, kCheckLen), 0xF4);
    EXPECT_EQ(CheckUtils::crc16CCITT(kCheck, kCheckLen), 0x29B1);
    EXPECT_EQ(CheckUtils::crc16XMODEM(kCheck, kCheckLen), 0x31C3);
    EXPECT_EQ(CheckUtils::crc16KERMIT(kCheck, kCheckLen), 0x2189);
    EXPECT_EQ(CheckUtils::crc16MODBUS(kCheck, kCheckLen), 0x4B37);
    EXPECT_EQ(CheckUtils::crc16ANSI(kCheck, kCheckLen), 0xBB3D);
    EXPECT_EQ(CheckUtils::crc16IBM(kCheck, kCheckLen), 0xBB3D);
    EXPECT_TRUE(CheckUtils::crc32IEEE(kCheck, kCheckLen) == 0xCBF43926u);
}
