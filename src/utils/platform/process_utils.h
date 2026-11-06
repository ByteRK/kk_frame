/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-05 15:17:45
 * @FilePath: /kk_frame/src/utils/platform/process_utils.h
 * @Description: 进程相关的一些函数（同步、退出、重启、执行命令）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PROCESS_UTILS_H__
#define __PROCESS_UTILS_H__

#include <cstddef>
#include <string>

/// @brief 进程级操作：数据同步、退出/重启、执行外部命令
namespace ProcessUtils {

    /// @brief 命令执行结果
    struct CommandResult {
        bool        started{ false };   // 是否成功启动子进程
        bool        finished{ false };  // 是否正常结束（未被超时杀进程）
        int         exitCode{ -1 };     // 退出码；被信号终止或未启动时为 -1
        int         termSignal{ 0 };    // 终止进程的信号；0 表示正常退出
        bool        timedOut{ false };  // 是否因超时被强制终止
        bool        truncated{ false }; // 输出是否因超过上限被截断
        std::string output;             // stdout 与 stderr 的合并内容

        /// @brief 命令是否成功（已结束、无信号、退出码为 0）
        bool success() const {
            return started && finished && !timedOut && termSignal == 0 && exitCode == 0;
        }
    };

    /// @brief 执行命令的默认超时（ms）
    static constexpr int DEFAULT_TIMEOUT_MS = 10000;

    /// @brief 捕获输出的默认上限（字节），防止命令刷屏耗尽内存
    static constexpr size_t DEFAULT_MAX_OUTPUT = 1024 * 1024;

    /// @brief 数据同步（将内存数据落盘）
    void sync();

    /// @brief 结束程序（退出前会自动 sync）
    /// @note 需要持久化的应用状态（如时间缓存）不会由本函数保存，
    ///       请在调用前自行写入
    void exit();

    /// @brief 重启设备（x64 直接退出进程），重启前会自动 sync
    /// @note 同 exit，调用前请自行保存需要持久化的应用状态
    void reboot();

    /// @brief 执行命令并返回输出
    /// @param cmd 命令
    /// @return 命令输出（已去除尾部空白）；启动失败或超时返回空字符串
    /// @note 需要判断退出码或区分超时时请用 runCommand
    std::string run(const std::string& cmd);

    /// @brief 异步执行系统命令（fork + exec，不等待，不捕获输出）
    /// @param cmd 命令
    /// @return true 已成功派发
    /// @note 内部采用两次 fork：中间进程立即被本函数回收，真正的命令进程脱离父子
    ///       关系后由 init 收养，因此不会产生僵尸进程，也不会超时
    /// @note 不关心退出码/输出；需要时请用 runCommand
    bool runAsync(const std::string& cmd);

    /// @brief 执行命令并捕获输出（带超时与退出码）
    /// @param cmd 命令（由 /bin/sh -c 执行）
    /// @param timeoutMs 超时（ms），<= 0 表示使用 DEFAULT_TIMEOUT_MS
    /// @param maxOutput 输出上限（字节），<= 0 表示使用 DEFAULT_MAX_OUTPUT
    /// @return 执行结果，含退出码、是否超时、合并后的输出
    /// @note 超时会发送 SIGKILL 并回收子进程，因此本函数不会无限阻塞
    ///       stdout 与 stderr 合并捕获，顺序与子进程写出顺序一致
    CommandResult runCommand(const std::string& cmd, int timeoutMs = DEFAULT_TIMEOUT_MS, size_t maxOutput = DEFAULT_MAX_OUTPUT);

} // ProcessUtils

#endif // !__PROCESS_UTILS_H__
