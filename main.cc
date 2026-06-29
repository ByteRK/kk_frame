/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-06-30 01:29:58
 * @FilePath: /kk_frame/main.cc
 * @Description: 主程序入口
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#if 1
/**
 * 如果需要不运行程序获取版本号相关信息
 * 
 * 直接调用 strings ./app | grep AVS: 
 */
#include "app_version.h"
static const char avs[] = "AVS: " APP_VER_INFO;
#endif

#include "custom_app.h"      // 自定义APP

#include "arg_utils.h"       // 参数工具集
#include "project_utils.h"   // 项目工具集

#include "tick_mgr.h"        // 心跳管理器
#include "global_data.h"     // 数据管理器
#include "config_mgr.h"      // 配置管理器
#include "history_mgr.h"     // 历史管理器
#include "statistics_mgr.h"  // 统计管理器
#include "thread_mgr.h"      // 线程管理器
#include "message_mgr.h"     // 消息管理器
#include "timer_mgr.h"       // 定时管理器
#include "work_mgr.h"        // 工作管理器
#include "wind_mgr.h"        // 窗口管理器

#include "wifi_mgr.h"        // WIFI管理器
#include "http_mgr.h"        // HTTP管理器

#include "conn_mgr.h"        // 电控通讯
#include "btn_mgr.h"         // 按键板通讯
#include "tuya_mgr.h"        // 涂鸦模组通讯

/// @brief 主函数
/// @param argc 参数个数
/// @param argv 参数列表
/// @return 返回值
int main(int argc, const char* argv[]) {
    ProjectUtils::env();              // 设定环境变量（x64生效）
    ProjectUtils::pInfo(argc, argv);  // 打印项目信息
    ProjectUtils::pKeyMap();          // 打印项目按键映射
    ArgUtils::parse(argc, argv);      // 解析命令行参数

    /* 框架 */
    CustomApp app(argc, argv);

    /* 心跳 */
    g_tick->init();

    /* 数据与统计 */
    g_data->init();
    g_config->init();
    g_history->init();
    g_statistics->init();

    /* 线程 */
    g_threadMgr->init(3);

    /* 消息 */
    g_msg->init();

    /* 工作 */
    g_timer->init();
    g_work->init();

    /* 网络 */
#if ENABLED(WIFI)
    g_wifi->init();
#endif

    /* HTTP请求 */
#if ENABLED(CURL)
    g_http->init(nullptr, 4, 1000);
#endif

    /* 通讯 */
    // g_connMgr->init();
    // g_btnMgr->init();
    // g_tuyaMgr->init();

    /* 页面 */
    g_windMgr->init();

    return app.exec();
}
