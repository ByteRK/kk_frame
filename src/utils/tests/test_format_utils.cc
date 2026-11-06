/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-05 19:25:00
 * @LastEditTime: 2026-11-05 19:25:00
 * @FilePath: /kk_frame/src/utils/tests/test_format_utils.cc
 * @Description: FormatUtils 用例（hex 展示、容量单位）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "format_utils.h"

TEST_CASE(format_hexStr) {
    const unsigned char data[] = { 0x00, 0x0A, 0xFF };
    EXPECT_STREQ(FormatUtils::hexStr(data, 3, 0), "00 0a ff ");
    // width 为 2 时每两个字节换行
    EXPECT_STREQ(FormatUtils::hexStr(data, 3, 2), "00 0a \nff ");
    // 非法输入
    EXPECT_STREQ(FormatUtils::hexStr(data, 0, 0), "");
    EXPECT_STREQ(FormatUtils::hexStr(nullptr, 3, 0), "");
}

TEST_CASE(format_size) {
    EXPECT_STREQ(FormatUtils::sizeVisualization(0), "0 B");
    EXPECT_STREQ(FormatUtils::sizeVisualization(512), "512 B");
    EXPECT_STREQ(FormatUtils::sizeVisualization(1024), "1.00 KB");
    EXPECT_STREQ(FormatUtils::sizeVisualization(1536, 1), "1.5 KB");
    EXPECT_STREQ(FormatUtils::sizeVisualization(1024ull * 1024 * 1024 * 1024 * 2, 0, true), "2 TB");
    // 不允许进位到 TB 时停在 GB
    EXPECT_STREQ(FormatUtils::sizeVisualization(1024ull * 1024 * 1024 * 1024 * 2, 0, false), "2048 GB");
}
