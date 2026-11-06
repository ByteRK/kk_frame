/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 17:00:00
 * @LastEditTime: 2026-11-06 09:58:45
 * @FilePath: /kk_frame/src/template/scope_guard.h
 * @Description: 作用域退出时执行指定动作（RAII 收尾）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __SCOPE_GUARD_H__
#define __SCOPE_GUARD_H__

#include <functional>
#include <utility>

/// @brief 作用域守卫：析构时执行给定动作
/// @note 用于替代"每个 return 前都要手写清理"的写法（C++11 无标准 defer）。
/// @note 本模块属于 utils/base，只依赖标准库，可直接被各层复用。
///
/// 用法：
///   FILE* fp = fopen(path, "r");
///   if (!fp) return false;
///   ScopeGuard closeFile([fp]() { fclose(fp); });
///   ...
///   return true;                       // 自动 fclose
///
///   // 需要提前释放时：
///   closeFile.release();               // 解除守卫，不再执行动作
///
///   // 需要提前执行时：
///   closeFile.invoke();
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> action)
        : mAction(std::move(action)), mActive(true) { }

    ~ScopeGuard() {
        if (mActive && mAction) {
            mAction();
        }
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ScopeGuard(ScopeGuard&& other) noexcept
        : mAction(std::move(other.mAction)), mActive(other.mActive) {
        other.mActive = false;
    }

    ScopeGuard& operator=(ScopeGuard&& other) noexcept {
        if (this != &other) {
            // 先执行当前守卫已持有的动作，避免资源泄漏
            if (mActive && mAction) mAction();
            mAction = std::move(other.mAction);
            mActive = other.mActive;
            other.mActive = false;
        }
        return *this;
    }

public:
    /// @brief 解除守卫（析构时不再执行动作）
    void release() {
        mActive = false;
    }

    /// @brief 立即执行动作并解除守卫（幂等）
    void invoke() {
        if (mActive) {
            mActive = false;
            if (mAction) mAction();
        }
    }

    /// @brief 守卫是否仍处于生效状态
    bool active() const {
        return mActive;
    }

private:
    std::function<void()> mAction; // 退出时执行的动作
    bool                  mActive; // 是否仍生效
};

/// @brief 便捷构造（C++11 无类模板参数推导，需借助工厂函数）
/// @param action 退出时执行的动作
/// @return 守卫对象
inline ScopeGuard makeScopeGuard(std::function<void()> action) {
    return ScopeGuard(std::move(action));
}

/// @brief 在作用域结束时执行固定语句（类似 Go 的 defer）
/// @note 变量名通过行号粘连保证唯一，同一行不可重复使用
#define DEFER_CONCAT_(a, b) a##b
#define DEFER_CONCAT(a, b)  DEFER_CONCAT_(a, b)
#define DEFER(code) \
    ::ScopeGuard DEFER_CONCAT(defer_guard_, __LINE__)([&]() { code; })

#endif // !__SCOPE_GUARD_H__
