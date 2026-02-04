/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-02-04 12:38:14
 * @FilePath: /kk_frame/main.cc
 * @Description: 主程序入口
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include <cdlog.h>           // 日志
#include <core/app.h>        // Cdroid应用

#include "arg_utils.h"
#include "project_utils.h"   // 项目工具集
#include "global_data.h"     // 全局数据

#include "config_mgr.h"      // 配置管理器
#include "history_mgr.h"     // 历史记录管理器
#include "statistics_mgr.h"  // 统计管理器
#include "thread_mgr.h"      // 线程管理器
#include "timer_mgr.h"       // 定时器管理器
#include "work_mgr.h"        // 工作管理器
#include "wind_mgr.h"        // 窗口管理器

#include "conn_mgr.h"        // 电控通讯
#include "btn_mgr.h"         // 按键板通讯
#include "tuya_mgr.h"        // 涂鸦模组通讯

#include "wifi_adapter.h"    // WIFI适配器

/// @brief 主函数
/// @param argc 参数个数
/// @param argv 参数列表
/// @return 返回值
int main(int argc, const char* argv[]) {
    ProjectUtils::env();           // 设定环境变量（x64生效）
    ProjectUtils::pInfo(argv[0]);  // 打印项目信息
    ArgUtils::parse(argc, argv);   // 解析命令行参数
    ProjectUtils::pKeyMap();       // 打印项目按键映射

    App app(argc, argv);
    cdroid::Context* ctx = &app;

    g_data->init();

    g_config->init();
    g_history->init();
    g_statistics->init();
    g_threadMgr->init(3);
    g_timer->init();
    g_work->init();
    g_windMgr->init();

    // g_connMgr->init();
    // g_btnMgr->init();
    // g_tuyaMgr->init();

#if ENABLE_WIFI
    WIFIAdapter::instance()->autoCheck();
#endif
    return app.exec();
}

