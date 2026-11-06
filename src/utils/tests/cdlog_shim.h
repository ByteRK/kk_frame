/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/cdlog_shim.h
 * @Description: 测试环境下的 cdlog 兼容层（把 utils 的日志调用接到 LogUtils）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TESTS_CDLOG_SHIM_H__
#define __TESTS_CDLOG_SHIM_H__

/// @note 本文件刻意不叫 cdlog.h：源码树里一旦出现同名头，编辑器（IntelliSense）
///       会把 #include <cdlog.h> 解析到它，从而跳不到框架的 cdlog.h。
///       实际用于顶替框架头的 cdlog.h 由测试工程的 CMake 在构建目录生成，
///       内容仅一行 #include "cdlog_shim.h"，不进入源码树。
///       作用：让 utils 保持"直接依赖 cdlog"的写法，而测试时无需引入 cdroid。
///
/// 兼容范围：
///   LOGV/LOGD/LOGI/LOGW/LOGE(...)      printf 风格
///   LOG(DEBUG) << 任意内容              流式风格
#include "log_utils.h"

#include <sstream>

enum LogLevelShim {
    LOG_VERBOSE = 0,
    LOG_DEBUG   = 1,
    LOG_INFO    = 2,
    LOG_WARN    = 3,
    LOG_ERROR   = 4,
    LOG_FATAL   = 5,
};

namespace tests_shim {

    /// @brief 级别转换
    /// @param level cdlog 风格级别
    /// @return LogUtils 级别
    inline LogUtils::Level toLevel(int level) {
        switch (level) {
        case LOG_DEBUG: return LogUtils::Level::Debug;
        case LOG_INFO:  return LogUtils::Level::Info;
        case LOG_WARN:  return LogUtils::Level::Warn;
        case LOG_ERROR: return LogUtils::Level::Error;
        case LOG_FATAL: return LogUtils::Level::Error;
        default:        return LogUtils::Level::Verbose;
        }
    }

    /// @brief 流式日志：析构时一次性输出
    class LogStream {
    public:
        LogStream(LogUtils::Level level, const char* file, int line)
            : mLevel(level), mFile(file), mLine(line) { }

        ~LogStream() {
            LogUtils::write(mLevel, mFile, mLine, "%s", mStream.str().c_str());
        }

        std::ostringstream& stream() {
            return mStream;
        }

    private:
        LogUtils::Level    mLevel;  // 日志级别
        const char*        mFile;   // 调用点文件
        int                mLine;   // 调用点行号
        std::ostringstream mStream; // 累积内容
    };

} // namespace tests_shim

#ifndef LOGV
#define LOGV(...) LogUtils::write(LogUtils::Level::Verbose, __FILE__, __LINE__, __VA_ARGS__)
#endif
#ifndef LOGD
#define LOGD(...) LogUtils::write(LogUtils::Level::Debug,   __FILE__, __LINE__, __VA_ARGS__)
#endif
#ifndef LOGI
#define LOGI(...) LogUtils::write(LogUtils::Level::Info,    __FILE__, __LINE__, __VA_ARGS__)
#endif
#ifndef LOGW
#define LOGW(...) LogUtils::write(LogUtils::Level::Warn,    __FILE__, __LINE__, __VA_ARGS__)
#endif
#ifndef LOGE
#define LOGE(...) LogUtils::write(LogUtils::Level::Error,   __FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef LOG
#define LOG(level) \
    tests_shim::LogStream(tests_shim::toLevel(LOG_##level), __FILE__, __LINE__).stream()
#endif

#endif // !__TESTS_CDLOG_SHIM_H__
