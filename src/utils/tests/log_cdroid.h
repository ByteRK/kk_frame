/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/log_cdroid.h
 * @Description: 测试环境下的日志兼容层（无 cdroid 依赖）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __LOG_CDROID_H__
#define __LOG_CDROID_H__

/// @brief 日志"框架接入"的兼容占位
/// @note 正式工程里该模块把 LogUtils 接到 cdroid 的 cdlog；
///       测试工程没有 cdroid，故这里只保留同名接口，
///       使被测代码在两种环境下都能编译，无需条件编译。
namespace CdroidLog {

    /// @brief 安装日志接收器
    /// @note 测试环境下为空操作：LogUtils 保持默认的 stderr 输出
    void install();

} // CdroidLog

#endif // !__LOG_CDROID_H__
