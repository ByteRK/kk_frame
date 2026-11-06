/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 16:00:00
 * @LastEditTime: 2026-11-04 16:00:00
 * @FilePath: /kk_frame/src/utils/tests/log_utils.h
 * @Description: 日志门面（屏蔽具体日志框架，供 utils 各层使用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __LOG_UTILS_H__
#define __LOG_UTILS_H__

/// @brief 日志门面
/// @note 设计目的：utils 的 L0/L1 层不应该依赖具体日志框架（cdroid/cdlog）。
///       本门面提供 printf 风格的 LOGV/LOGD/LOGI/LOGW/LOGE，
///       实际输出由 Sink 决定：
///       - 默认 Sink 打到 stderr（无框架依赖，便于单元测试与独立复用）
///       - 应用启动时可安装框架 Sink（见 integration/log_cdroid.h）
///
/// 用法（与原有 cdlog 宏一致，无需学习成本）：
///   LOGI("value=%d", v);
///   LOGE("open failed. path=%s errno=%d", path, errno);
namespace LogUtils {

    /// @brief 日志级别（数值与 cdlog 的 LOG_* 顺序无关，桥接层自行映射）
    enum class Level {
        Verbose = 0,
        Debug   = 1,
        Info    = 2,
        Warn    = 3,
        Error   = 4,
    };

    /// @brief 日志接收器
    /// @param level 级别
    /// @param file 调用点文件
    /// @param line 调用点行号
    /// @param message 已格式化好的消息（不含换行）
    typedef void (*Sink)(Level level, const char* file, int line, const char* message);

    /// @brief 安装日志接收器
    /// @param sink 接收器，传 nullptr 恢复默认的 stderr 输出
    /// @note 非线程安全，必须在多线程开始工作前调用
    void setSink(Sink sink);

    /// @brief 获取当前接收器
    /// @return 当前 Sink；未安装时返回默认 stderr Sink
    Sink sink();

    /// @brief 写出日志
    /// @param level 级别
    /// @param file 调用点文件
    /// @param line 调用点行号
    /// @param format printf 风格格式串
    void write(Level level, const char* file, int line, const char* format, ...)
#if defined(__GNUC__)
        __attribute__((format(printf, 4, 5)))
#endif
        ;

} // LogUtils

/* 与 cdroid 的 cdlog.h 同名，使用 #ifndef 保护，
 * 允许个别文件同时包含本头与框架头（框架宏优先，避免重定义告警） */
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

#endif // !__LOG_UTILS_H__
