/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/test_string_utils.cc
 * @Description: StringUtils 用例（基础操作 / 数值解析 / UTF-8 / 编码转换）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "test_framework.h"

#include "string_utils.h"


TEST_CASE(string_trim) {
    EXPECT_STREQ(StringUtils::trim("  abc  "), "abc");
    EXPECT_STREQ(StringUtils::trim("\r\n\t abc \t\r\n"), "abc");
    EXPECT_STREQ(StringUtils::trimLeft("  abc"), "abc");
    EXPECT_STREQ(StringUtils::trimRight("abc  "), "abc");
    EXPECT_STREQ(StringUtils::trim(""), "");

    std::string value = "  abc  ";
    StringUtils::trimInPlace(value);
    EXPECT_STREQ(value, "abc");
}

TEST_CASE(string_toBool) {
    bool value = false;

    EXPECT_TRUE(StringUtils::toBool("1", value));
    EXPECT_TRUE(value);
    EXPECT_TRUE(StringUtils::toBool("YES", value));
    EXPECT_TRUE(value);
    EXPECT_TRUE(StringUtils::toBool(" enabled ", value));
    EXPECT_TRUE(value);

    EXPECT_TRUE(StringUtils::toBool("off", value));
    EXPECT_FALSE(value);

    EXPECT_FALSE(StringUtils::toBool("maybe", value));
}

TEST_CASE(string_toNumber) {
    int intValue = 0;
    EXPECT_TRUE(StringUtils::toInt(" 42 ", intValue));
    EXPECT_EQ(intValue, 42);
    EXPECT_TRUE(StringUtils::toInt("-7", intValue));
    EXPECT_EQ(intValue, -7);

    // 整串必须合法
    EXPECT_FALSE(StringUtils::toInt("42abc", intValue));
    EXPECT_FALSE(StringUtils::toInt("", intValue));
    // 溢出必须被拒绝
    EXPECT_FALSE(StringUtils::toInt("99999999999", intValue));

    int64_t bigValue = 0;
    EXPECT_TRUE(StringUtils::toInt64("9223372036854775807", bigValue));
    EXPECT_TRUE(bigValue == 9223372036854775807LL);

    double doubleValue = 0.0;
    EXPECT_TRUE(StringUtils::toDouble("1.5", doubleValue));
    EXPECT_TRUE(doubleValue > 1.49 && doubleValue < 1.51);
    EXPECT_FALSE(StringUtils::toDouble("1.5x", doubleValue));
}

TEST_CASE(string_splitJoin) {
    const std::vector<std::string> parts = StringUtils::split("a,b,,c", ',');
    EXPECT_EQ(parts.size(), static_cast<size_t>(3));
    EXPECT_STREQ(parts[0], "a");
    EXPECT_STREQ(parts[2], "c");

    const std::vector<std::string> any = StringUtils::splitAny("a b\tc,d", " \t,");
    EXPECT_EQ(any.size(), static_cast<size_t>(4));
    EXPECT_STREQ(any[3], "d");

    EXPECT_STREQ(StringUtils::join(any, "-"), "a-b-c-d");

    // join 与 split 对称
    const std::vector<std::string> back = StringUtils::split(StringUtils::join(parts, ","), ',');
    EXPECT_EQ(back.size(), parts.size());
}

TEST_CASE(string_prefixSuffix) {
    EXPECT_TRUE(StringUtils::startsWith("abcdef", "abc"));
    EXPECT_FALSE(StringUtils::startsWith("abcdef", "abd"));
    EXPECT_TRUE(StringUtils::endsWith("abcdef", "def"));
    EXPECT_TRUE(StringUtils::endsWithIgnoreCase("abcdef", "DEF"));
    EXPECT_TRUE(StringUtils::equalsIgnoreCase("AbC", "aBc"));
    EXPECT_FALSE(StringUtils::equalsIgnoreCase("abc", "ab"));
}

TEST_CASE(string_formatFill) {
    EXPECT_STREQ(StringUtils::format("%d-%s", 7, "x"), "7-x");
    EXPECT_STREQ(StringUtils::fill(7, 3, '0'), "007");
    EXPECT_STREQ(StringUtils::fill("ab", 4, '.'), "ab..");
}

TEST_CASE(utf8_countAndTruncate) {
    // 中文按权重 2 计：3 个汉字 = 6
    EXPECT_EQ(StringUtils::characterCount("中文测", 1), static_cast<size_t>(3));
    EXPECT_EQ(StringUtils::characterCount("中文测", 2), static_cast<size_t>(6));
    EXPECT_EQ(StringUtils::characterCount("ab中", 2), static_cast<size_t>(4));

    // 不应截断半个汉字
    EXPECT_STREQ(StringUtils::substringByChars("中文测试", 2, 1), "中文");
    EXPECT_STREQ(StringUtils::substringByChars("中文测试", 3, 2), "中");
    EXPECT_STREQ(StringUtils::substringByChars("abcdef", 3, 1), "abc");
}

TEST_CASE(utf8_removeLast) {
    EXPECT_STREQ(StringUtils::removeLastCharacter("abc"), "ab");
    EXPECT_STREQ(StringUtils::removeLastCharacter("中文"), "中");
    EXPECT_STREQ(StringUtils::removeLastCharacter("a"), "");
    EXPECT_STREQ(StringUtils::removeLastCharacter(""), "");

    std::string value = "中文";
    StringUtils::popLastCharacter(value);
    EXPECT_STREQ(value, "中");

    std::string single = "a";
    StringUtils::popLastCharacter(single);
    EXPECT_STREQ(single, "");
}

TEST_CASE(utf8_truncateWithLimit) {
    EXPECT_STREQ(StringUtils::truncateWithLimit("abcdef", 6), "abcdef");
    EXPECT_STREQ(StringUtils::truncateWithLimit("abcdef", 4, 1, ".."), "ab..");
    // 后缀本身已超出上限：直接截断后缀到上限长度
    EXPECT_STREQ(StringUtils::truncateWithLimit("abcdef", 2, 1, "..."), "..");
}

TEST_CASE(utf8_popLastWithEmbeddedNul) {
    // 内嵌 '\0' 不得被当作串尾，只应删掉最后一个字符
    std::string value("ab\0cd", 5);
    StringUtils::popLastCharacter(value);
    EXPECT_EQ(value.size(), static_cast<size_t>(4));
    EXPECT_EQ(value[0], 'a');
    EXPECT_EQ(value[2], '\0');
    EXPECT_EQ(value[3], 'c');
}

TEST_CASE(string_isNumeric) {
    EXPECT_TRUE(StringUtils::isNumeric("0"));
    EXPECT_TRUE(StringUtils::isNumeric("-12.5"));
    EXPECT_FALSE(StringUtils::isNumeric(""));
    EXPECT_FALSE(StringUtils::isNumeric(nullptr));
    EXPECT_FALSE(StringUtils::isNumeric("1.2.3"));
    EXPECT_FALSE(StringUtils::isNumeric("abc"));
    EXPECT_FALSE(StringUtils::isNumeric("-"));
    // 高位字节（负 char）不得触发未定义行为
    EXPECT_FALSE(StringUtils::isNumeric("\xE4\xB8\xAD"));
}
