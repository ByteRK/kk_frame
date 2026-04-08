/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-04-09 00:59:04
 * @FilePath: /kk_frame/main.cc
 * @Description: 主程序入口
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include <cdlog.h>           // 日志
#include "custom_app.h"      // 自定义应用程序
#include "src/common.h"      // 公共头文件

#include "arg_utils.h"
#include "project_utils.h"   // 项目工具集
#include "global_data.h"     // 全局数据

#include "config_mgr.h"      // 配置管理器
#include "history_mgr.h"     // 历史记录管理器
#include "statistics_mgr.h"  // 统计管理器
#include "thread_mgr.h"      // 线程管理器
#include "message_mgr.h"     // 消息管理器
#include "timer_mgr.h"       // 定时器管理器
#include "work_mgr.h"        // 工作管理器
#include "wind_mgr.h"        // 窗口管理器

#include "wifi_mgr.h"        // WIFI
#include "http_mgr.h"        // HTTP管理器

#include "conn_mgr.h"        // 电控通讯
#include "btn_mgr.h"         // 按键板通讯
#include "tuya_mgr.h"        // 涂鸦模组通讯

/// @brief 主函数
/// @param argc 参数个数
/// @param argv 参数列表
/// @return 返回值
int main(int argc, const char* argv[]) {
    ProjectUtils::env();           // 设定环境变量（x64生效）
    ProjectUtils::pInfo(argv[0]);  // 打印项目信息
    ProjectUtils::pKeyMap();       // 打印项目按键映射
    ArgUtils::parse(argc, argv);   // 解析命令行参数

    /* 框架 */
    CustomApp app(argc, argv);

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
    // g_wifi->init();

    /* HTTP请求 */
    g_http->init(nullptr, 4, 1000);

    /* 通讯 */
    // g_connMgr->init();
    // g_btnMgr->init();
    // g_tuyaMgr->init();

    /* 页面 */
    g_windMgr->init();

    return app.exec();
}

