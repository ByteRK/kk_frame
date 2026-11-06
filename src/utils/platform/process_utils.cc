/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-05 15:18:27
 * @FilePath: /kk_frame/src/utils/platform/process_utils.cc
 * @Description: 进程相关的一些函数（同步、退出、重启、执行命令）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "process_utils.h"

#include "clock_utils.h"
#include <cdlog.h>
#include "string_utils.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

    /// @brief 从管道读取数据直到 EOF 或超时
    /// @param fd 读端
    /// @param deadlineMs 单调时钟截止时间
    /// @param maxOutput 输出上限
    /// @param result 写入 output / timedOut / truncated
    void drainPipe(int fd, int64_t deadlineMs, size_t maxOutput, ProcessUtils::CommandResult& result) {
        char buffer[1024];

        for (;;) {
            const int64_t remaining = deadlineMs - ClockUtils::monotonicMs();
            if (remaining <= 0) {
                result.timedOut = true;
                return;
            }

            struct pollfd item = {};
            item.fd = fd;
            item.events = POLLIN;

            const int pollResult = poll(&item, 1, static_cast<int>(remaining));
            if (pollResult < 0) {
                if (errno == EINTR) continue;
                LOGE("runCommand: poll failed. errno=%d", errno);
                return;
            }
            if (pollResult == 0) {
                // 到达截止时间仍未读到数据
                result.timedOut = true;
                return;
            }

            const ssize_t bytes = read(fd, buffer, sizeof(buffer));
            if (bytes < 0) {
                if (errno == EINTR) continue;
                LOGE("runCommand: read failed. errno=%d", errno);
                return;
            }
            if (bytes == 0) {
                // EOF：子进程已关闭输出
                return;
            }

            // 超出上限的部分丢弃，避免命令刷屏耗尽内存
            const size_t allowed = (result.output.size() < maxOutput)
                ? (maxOutput - result.output.size()) : 0;
            if (static_cast<size_t>(bytes) > allowed) {
                result.truncated = true;
            }
            if (allowed > 0) {
                result.output.append(buffer, std::min(allowed, static_cast<size_t>(bytes)));
            }
        }
    }

    /// @brief 等待子进程结束，必要时在超时后强制终止
    /// @param pid 子进程
    /// @param deadlineMs 截止时间
    /// @param result 写入 finished / exitCode / termSignal
    void waitChild(pid_t pid, int64_t deadlineMs, ProcessUtils::CommandResult& result) {
        int status = 0;

        for (;;) {
            const pid_t waited = waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                break;
            }
            if (waited < 0) {
                if (errno == EINTR) continue;
                LOGE("runCommand: waitpid failed. errno=%d", errno);
                return;
            }

            // 子进程还在运行：到点就强制终止
            if (ClockUtils::monotonicMs() >= deadlineMs) {
                result.timedOut = true;
                kill(pid, SIGKILL);
                // SIGKILL 不可捕获，下面的阻塞等待必然很快返回
                while (waitpid(pid, &status, 0) < 0 && errno == EINTR) { }
                break;
            }

            usleep(5 * 1000);
        }

        if (WIFEXITED(status)) {
            result.finished = true;
            result.exitCode = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.finished = false;
            result.termSignal = WTERMSIG(status);
        }
    }

} // namespace

void ProcessUtils::sync() {
#ifndef PRODUCT_X64
    // 直接用系统调用，避免为一次 sync 额外 fork 一个 shell
    ::sync();
#else
    LOGI("-------- sync --------");
#endif
}

void ProcessUtils::exit() {
    // 退出前统一刷盘：调用方可能跳过显式 sync，这里兜底
    sync();
    ::exit(0);
}

void ProcessUtils::reboot() {
    // 重启前同样先刷盘，避免刚写入的状态随复位丢失
    sync();
#ifndef PRODUCT_X64
    std::system("reboot");
#else
    ::exit(0);
#endif
}

std::string ProcessUtils::run(const std::string& cmd) {
    const CommandResult result = runCommand(cmd);
    if (!result.started || result.timedOut) {
        return std::string();
    }
    return result.output;
}

bool ProcessUtils::runAsync(const std::string& cmd) {
    if (cmd.empty()) {
        LOGE("runAsync: empty command");
        return false;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        LOGE("runAsync: fork failed. errno=%d", errno);
        return false;
    }

    if (pid == 0) {
        // 子进程：两次 fork 让真正的命令进程被 init 收养。
        // 否则父进程不 waitpid 时，命令结束后会留下僵尸进程直到父进程退出。
        setsid();
        const pid_t grandChild = fork();
        if (grandChild < 0) ::_exit(127);
        if (grandChild > 0) ::_exit(0); // 中间进程立刻退出，由父进程回收

        execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        ::_exit(127); // 如果 execl 失败
    }

    // 父进程：只回收中间进程，真正的命令进程已脱离父子关系
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            LOGE("runAsync: waitpid failed. errno=%d", errno);
            return false;
        }
    }
    return true;
}

ProcessUtils::CommandResult ProcessUtils::runCommand(const std::string& cmd,
    int timeoutMs, size_t maxOutput) {
    CommandResult result;

    if (cmd.empty()) {
        LOGE("runCommand: empty command");
        return result;
    }

    if (timeoutMs <= 0) timeoutMs = DEFAULT_TIMEOUT_MS;
    if (maxOutput <= 0) maxOutput = DEFAULT_MAX_OUTPUT;

    const int64_t startMs = ClockUtils::monotonicMs();
    const int64_t deadlineMs = startMs + timeoutMs;

    int pipeFd[2] = { -1, -1 };
    if (pipe(pipeFd) != 0) {
        LOGE("runCommand: pipe failed. errno=%d", errno);
        return result;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        LOGE("runCommand: fork failed. errno=%d", errno);
        close(pipeFd[0]);
        close(pipeFd[1]);
        return result;
    }

    if (pid == 0) {
        // 子进程：stdout 与 stderr 都汇入管道
        close(pipeFd[0]);
        dup2(pipeFd[1], STDOUT_FILENO);
        dup2(pipeFd[1], STDERR_FILENO);
        close(pipeFd[1]);

        execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        ::_exit(127); // execl 失败
    }

    // 父进程
    result.started = true;
    close(pipeFd[1]);

    drainPipe(pipeFd[0], deadlineMs, maxOutput, result);
    close(pipeFd[0]);

    waitChild(pid, deadlineMs, result);

    StringUtils::trimInPlace(result.output);

    // 计时使用单调时钟，避免系统校时导致耗时出现负数
    const int64_t costMs = ClockUtils::monotonicMs() - startMs;
    if (result.success()) {
        if (costMs > 10) {
            LOGW("runCommand consume [%lld]ms [%s] [%s]",
                static_cast<long long>(costMs), cmd.c_str(), result.output.c_str());
        } else {
            LOGV("runCommand consume [%lld]ms [%s] [%s]",
                static_cast<long long>(costMs), cmd.c_str(), result.output.c_str());
        }
    } else if (result.timedOut) {
        LOGE("runCommand timeout [%lld]ms [%s] [%s]",
            static_cast<long long>(costMs), cmd.c_str(), result.output.c_str());
    } else {
        LOGW("runCommand failed. exit=%d signal=%d cost=[%lld]ms [%s] [%s]",
            result.exitCode, result.termSignal,
            static_cast<long long>(costMs), cmd.c_str(), result.output.c_str());
    }
    return result;
}
