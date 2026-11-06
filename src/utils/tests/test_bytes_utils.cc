/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-06 10:00:00
 * @FilePath: /kk_frame/src/utils/tests/test_bytes_utils.cc
 * @Description: BytesUtils 用例（字节序读写、hex 编解码）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "bytes_utils.h"

TEST_CASE(bytes_hexValue) {
    EXPECT_EQ(BytesUtils::hexValue('0'), 0);
    EXPECT_EQ(BytesUtils::hexValue('9'), 9);
    EXPECT_EQ(BytesUtils::hexValue('a'), 10);
    EXPECT_EQ(BytesUtils::hexValue('F'), 15);
    EXPECT_EQ(BytesUtils::hexValue('g'), -1);
    EXPECT_EQ(BytesUtils::hexValue(' '), -1);
}

TEST_CASE(bytes_hexEncodeDecode) {
    const std::string raw("\x00\x01\xab\xff", 4);
    EXPECT_STREQ(BytesUtils::hexEncode(raw.data(), raw.size()), "0001abff");
    EXPECT_STREQ(BytesUtils::hexEncode(raw), "0001abff");

    std::string decoded;
    EXPECT_TRUE(BytesUtils::hexDecode("0001abff", decoded));
    EXPECT_TRUE(decoded == raw);

    // 往返一致
    std::string roundTrip;
    EXPECT_TRUE(BytesUtils::hexDecode(BytesUtils::hexEncode(raw), roundTrip));
    EXPECT_TRUE(roundTrip == raw);

    // 允许分隔符
    EXPECT_TRUE(BytesUtils::hexDecode("AA:BB-CC", decoded, true));
    EXPECT_EQ(static_cast<unsigned>(decoded.size()), 3u);
    EXPECT_EQ(static_cast<unsigned char>(decoded[0]), 0xAAu);

    // 非法输入
    EXPECT_FALSE(BytesUtils::hexDecode("abc", decoded));
    EXPECT_FALSE(BytesUtils::hexDecode("zz", decoded));
    EXPECT_FALSE(BytesUtils::hexDecode("AA BB", decoded, false));
}

TEST_CASE(bytes_endianReadWrite) {
    unsigned char buffer[8] = { 0 };

    BytesUtils::writeBig32(0x12345678u, buffer);
    EXPECT_EQ(buffer[0], 0x12);
    EXPECT_EQ(buffer[3], 0x78);
    EXPECT_TRUE(BytesUtils::readBig32(buffer) == 0x12345678u);

    BytesUtils::writeLittle32(0x12345678u, buffer);
    EXPECT_EQ(buffer[0], 0x78);
    EXPECT_TRUE(BytesUtils::readLittle32(buffer) == 0x12345678u);

    // 指定字节序的读写
    BytesUtils::write16(0xABCD, buffer, BytesUtils::BigEndian);
    EXPECT_TRUE(BytesUtils::read16(buffer, BytesUtils::BigEndian) == 0xABCD);
    EXPECT_TRUE(BytesUtils::read16(buffer, BytesUtils::LittleEndian) == 0xCDAB);

    // 64 位往返
    BytesUtils::write64(0x1122334455667788ull, buffer, BytesUtils::LittleEndian);
    EXPECT_TRUE(BytesUtils::read64(buffer, BytesUtils::LittleEndian) == 0x1122334455667788ull);
}

TEST_CASE(bytes_bswap) {
    EXPECT_EQ(BytesUtils::bswap16(0x1234), 0x3412);
    EXPECT_TRUE(BytesUtils::bswap32(0x12345678u) == 0x78563412u);
    EXPECT_TRUE(BytesUtils::bswap64(0x1122334455667788ull) == 0x8877665544332211ull);
    EXPECT_TRUE(BytesUtils::isLittleEndian());
}
