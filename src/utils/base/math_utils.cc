/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 18:24:51
 * @LastEditTime: 2026-11-05 11:58:23
 * @FilePath: /kk_frame/src/utils/base/math_utils.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "math_utils.h"

#include <algorithm>
#include <atomic>
#include <random>

namespace {

    /// @brief 线程本地随机引擎（每个线程一份，互不干扰、无需加锁）
    /// @note 播种为惰性初始化，每个线程首次调用 rand() 时才构造引擎，之后仅有一次守卫位判断
    /// @note 本工程产物为可执行文件，thread_local 走 local-exec 模型，访问无额外函数调用；
    ///       若日后编进动态库，应补 __attribute__((tls_model("initial-exec")))，否则每次访问
    ///       可能退化为 __tls_get_addr() 调用。
    std::mt19937& randomEngine() {
        thread_local std::mt19937 engine([] {
            static std::atomic<unsigned> seq{ 0 };
            const unsigned id = ++seq;
            std::random_device rd;
            std::seed_seq seeds{ rd(), rd(), id };
            return std::mt19937(seeds);
        }());
        return engine;
    }

} // namespace

int MathUtils::rand(int min, int max) {
    if (min > max) std::swap(min, max);
    std::uniform_int_distribution<int> dis(min, max);
    return dis(randomEngine());
}

uint16_t MathUtils::toU16(uint8_t h, uint8_t l) {
    return static_cast<uint16_t>(h) << 8 | l;
}

uint32_t MathUtils::toU32(uint8_t h, uint8_t m, uint8_t l, uint8_t b) {
    return static_cast<uint32_t>(h) << 24
        | static_cast<uint32_t>(m) << 16
        | static_cast<uint32_t>(l) << 8
        | b;
}

int32_t MathUtils::rgb(uint8_t r, uint8_t g, uint8_t b) {
    return rgba(r, g, b, 0xFF);
}

int32_t MathUtils::rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return static_cast<int32_t>(static_cast<uint32_t>(a) << 24
        | static_cast<uint32_t>(r) << 16
        | static_cast<uint32_t>(g) << 8
        | b);
}
