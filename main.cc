/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2025-11-27 03:37:17
 * @FilePath: /cy_frame/main.cc
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#include <cdlog.h>
#include <core/app.h>
#include "fonts_info.h"
#include "series_info.h"

#include "this_func.h"
#include "global_data.h"
#include "config_mgr.h"
#include "timer_mgr.h"
#include "wind_mgr.h"

#include "conn_mgr.h"
#include "btn_mgr.h"
#include "tuya_mgr.h"

void setAppEnv() {
#ifdef CDROID_X64
    setenv("FONTCONFIG_PATH", FONTCONFIG_PATH, 1);
    setenv("SCREEN_SIZE", SCREEN_SIZE, 1);
#endif
}

int main(int argc, const char* argv[]) {
    setAppEnv();
    printProjectInfo(argv[0]);
    // printKeyMap();

    App app(argc, argv);
    cdroid::Context* ctx = &app;
    g_data->init();
    g_config->init();
    g_timerMgr->init();
    g_windMgr->init();
    // g_connMgr->init();
    // g_btnMgr->init();
    // g_tuyaMgr->init();

    return app.exec();
}

