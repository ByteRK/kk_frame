/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-05 17:25:00
 * @LastEditTime: 2026-11-05 17:25:00
 * @FilePath: /kk_frame/src/utils/tests/test_misc_utils.cc
 * @Description: ScopeGuard / MathUtils / StringUtils::convert / SysNodeUtils 用例
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "math_utils.h"
#include "scope_guard.h"
#include "string_utils.h"
#include "sysnode_utils.h"

TEST_CASE(scopeguard_basic) {
    int calls = 0;
    {
        ScopeGuard guard([&calls]() { ++calls; });
        EXPECT_EQ(calls, 0);
    }
    EXPECT_EQ(calls, 1);

    // release 后不再执行
    int released = 0;
    {
        ScopeGuard guard([&released]() { ++released; });
        guard.release();
        EXPECT_FALSE(guard.active());
    }
    EXPECT_EQ(released, 0);

    // invoke 立即执行且幂等
    int invoked = 0;
    {
        ScopeGuard guard([&invoked]() { ++invoked; });
        guard.invoke();
        guard.invoke();
        EXPECT_EQ(invoked, 1);
    }
    EXPECT_EQ(invoked, 1);
}

TEST_CASE(scopeguard_move) {
    int calls = 0;
    {
        ScopeGuard source([&calls]() { ++calls; });
        ScopeGuard target(makeScopeGuard([&calls]() { ++calls; }));
        target = std::move(source);
    }
    // 被覆盖的 target 先执行一次，随后 source 的动作各执行一次
    EXPECT_EQ(calls, 2);
}

TEST_CASE(math_basics) {
    // 闭区间
    for (int i = 0; i < 200; ++i) {
        const int value = MathUtils::rand(-3, 3);
        EXPECT_TRUE(value >= -3 && value <= 3);
    }

    // min > max 自动交换
    for (int i = 0; i < 50; ++i) {
        const int value = MathUtils::rand(5, 1);
        EXPECT_TRUE(value >= 1 && value <= 5);
    }

    EXPECT_EQ(MathUtils::toU16(0x12, 0x34), 0x1234);
    EXPECT_TRUE(MathUtils::toU32(0x11, 0x22, 0x33, 0x44) == 0x11223344u);
    EXPECT_TRUE(MathUtils::rgb(1, 2, 3) == MathUtils::rgba(1, 2, 3, 0xFF));
    EXPECT_TRUE((static_cast<uint32_t>(MathUtils::rgba(1, 2, 3, 0x80)) & 0xFF000000u) == 0x80000000u);
}

TEST_CASE(charset_convert) {
    // UTF-8 -> UTF-8 应保持不变（含中文）
    EXPECT_STREQ(StringUtils::convert("中文ab", "UTF-8", "UTF-8"), "中文ab");
    // ASCII 子集在 latin1 下等价
    EXPECT_STREQ(StringUtils::convert("abc", "UTF-8", "ISO-8859-1"), "abc");
    // 非法编码名 / 空输入
    EXPECT_STREQ(StringUtils::convert("abc", nullptr, "UTF-8"), "");
    EXPECT_STREQ(StringUtils::convert("abc", "UTF-8", nullptr), "");
    EXPECT_STREQ(StringUtils::convert("", "UTF-8", "UTF-8"), "");
    EXPECT_STREQ(StringUtils::convert("abc", "NO-SUCH-CHARSET", "UTF-8"), "");
}

TEST_CASE(charset_convertLargeInputNeedsGrowth) {
    // 长串 + 目标编码更宽，用于覆盖 E2BIG 扩容分支
    std::string input;
    for (int i = 0; i < 2048; ++i) input += "A";
    const std::string converted = StringUtils::convert(input, "UTF-8", "UTF-32BE");
    EXPECT_EQ(converted.size(), input.size() * 4);
}

TEST_CASE(proc_readTextFile) {
    std::string content;
    EXPECT_TRUE(SysNodeUtils::readTextFile("/proc/self/status", content));
    EXPECT_TRUE(content.find("Name:") != std::string::npos);

    // 不存在时返回 false 且不崩溃
    EXPECT_FALSE(SysNodeUtils::readTextFile("/proc/no-such-node-xyz", content));
}
