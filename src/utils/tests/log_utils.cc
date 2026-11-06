/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 16:00:00
 * @LastEditTime: 2026-11-04 16:00:00
 * @FilePath: /kk_frame/src/utils/base/log_utils.cc
 * @Description: 日志门面（屏蔽具体日志框架，供 utils 各层使用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include <cdlog.h>

#include <cstdarg>
#include <cstdio>
#include <vector>

namespace {

    /// @brief 默认接收器：输出到 stderr
    /// @note 仅在未安装框架 Sink 时生效（单元测试、独立工具、框架初始化前）
    void defaultSink(LogUtils::Level level, const char* file, int line, const char* message) {
        static const char* const kTags[] = { "V", "D", "I", "W", "E" };
        const int index = static_cast<int>(level);
        const char* tag = (index >= 0 && index < 5) ? kTags[index] : "?";

        std::fprintf(stderr, "[%s][%s:%d] %s\n", tag, file == nullptr ? "-" : file, line, message);
    }

    /// @brief 当前接收器（默认 stderr）
    LogUtils::Sink sSink = &defaultSink;

} // namespace

void LogUtils::setSink(Sink sink) {
    sSink = (sink != nullptr) ? sink : &defaultSink;
}

LogUtils::Sink LogUtils::sink() {
    return sSink;
}

void LogUtils::write(Level level, const char* file, int line, const char* format, ...) {
    if (format == nullptr) {
        return;
    }

    va_list args;
    va_start(args, format);
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int size = std::vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);

    if (size < 0) {
        va_end(args);
        sSink(level, file, line, format);
        return;
    }

    std::vector<char> buffer(static_cast<size_t>(size) + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);

    sSink(level, file, line, buffer.data());
}
