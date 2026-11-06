/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/test_framework.h
 * @Description: 极简断言框架（不引入第三方测试库）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TEST_FRAMEWORK_H__
#define __TEST_FRAMEWORK_H__

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace testfw {

    /// @brief 单个用例的登记项
    struct Case {
        const char* name;
        void (*fn)();
    };

    /// @brief 全局用例表
    inline std::vector<Case>& cases() {
        static std::vector<Case> list;
        return list;
    }

    /// @brief 全局统计
    struct Stats {
        int passed{ 0 };
        int failed{ 0 };
        int currentFailed{ 0 };
        const char* currentCase{ "" };
    };

    inline Stats& stats() {
        static Stats s;
        return s;
    }

    /// @brief 登记一条失败信息
    inline void fail(const char* file, int line, const std::string& message) {
        ++stats().failed;
        ++stats().currentFailed;
        std::fprintf(stderr, "    FAIL %s:%d  %s\n", file, line, message.c_str());
    }

    /// @brief 执行全部用例
    /// @return 进程退出码（0 全通过）
    inline int run() {
        for (size_t i = 0; i < cases().size(); ++i) {
            const Case& item = cases()[i];
            stats().currentCase = item.name;
            stats().currentFailed = 0;

            std::printf("[ RUN  ] %s\n", item.name);
            item.fn();

            if (stats().currentFailed == 0) {
                ++stats().passed;
                std::printf("[  OK  ] %s\n", item.name);
            } else {
                std::printf("[ FAIL ] %s\n", item.name);
            }
        }

        std::printf("\n==== %d passed, %d failed ====\n", stats().passed, stats().failed);
        return stats().failed == 0 ? 0 : 1;
    }

    /// @brief 用例登记器
    struct Registrar {
        Registrar(const char* name, void (*fn)()) {
            cases().push_back(Case{ name, fn });
        }
    };

} // namespace testfw

/// @brief 定义一个测试用例
#define TEST_CASE(name)                                                    \
    static void name();                                                    \
    static ::testfw::Registrar test_registrar_##name(#name, &name);        \
    static void name()

/// @brief 断言条件成立
#define EXPECT_TRUE(cond)                                                  \
    do {                                                                   \
        if (!(cond)) {                                                     \
            ::testfw::fail(__FILE__, __LINE__, "expect true: " #cond);     \
        }                                                                  \
    } while (0)

/// @brief 断言条件不成立
#define EXPECT_FALSE(cond)                                                 \
    do {                                                                   \
        if (cond) {                                                        \
            ::testfw::fail(__FILE__, __LINE__, "expect false: " #cond);    \
        }                                                                  \
    } while (0)

/// @brief 断言两值相等（失败时打印双方取值）
#define EXPECT_EQ(actual, expected)                                        \
    do {                                                                   \
        const auto& actualValue = (actual);                                \
        const auto& expectedValue = (expected);                            \
        if (!(actualValue == expectedValue)) {                             \
            std::ostringstream stream;                                     \
            stream << "expect " #actual " == " #expected                 \
                   << " but got " << actualValue << " vs " << expectedValue; \
            ::testfw::fail(__FILE__, __LINE__, stream.str());              \
        }                                                                  \
    } while (0)

/// @brief 断言两字符串相等（失败时打印内容）
#define EXPECT_STREQ(actual, expected)                                     \
    do {                                                                   \
        const std::string actualValue = (actual);                          \
        const std::string expectedValue = (expected);                      \
        if (actualValue != expectedValue) {                                \
            ::testfw::fail(__FILE__, __LINE__,                             \
                "expect \"" + expectedValue + "\" but got \""              \
                + actualValue + "\"");                                     \
        }                                                                  \
    } while (0)

#endif // !__TEST_FRAMEWORK_H__
