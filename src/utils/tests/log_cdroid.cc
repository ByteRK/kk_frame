/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 18:30:00
 * @LastEditTime: 2026-11-04 18:30:00
 * @FilePath: /kk_frame/src/utils/tests/log_cdroid.cc
 * @Description: 测试环境下的日志兼容层（无 cdroid 依赖）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "log_cdroid.h"

#include "log_utils.h"

void CdroidLog::install() {
    // 测试环境无需接入外部日志框架：恢复默认 Sink（输出到 stderr）。
    // 这样即使被测代码调用了 install()，日志仍可见且不会因缺少框架而失败。
    LogUtils::setSink(nullptr);
}
