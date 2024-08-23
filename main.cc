/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2024-08-23 13:52:30
 * @FilePath: /kk_frame/main.cc
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#include <cdlog.h>
#include <core/app.h>

#include "this_func.h"
#include "config_mgr.h"
#include "conn_mgr.h"
#include "btn_mgr.h"
#include "tuya_mgr.h"
#include "manage.h"

int main(int argc, const char* argv[]) {
    printProjectInfo(argv[0]);

    App app(argc, argv);
    cdroid::Context* ctx = &app;
    g_data->init();
    g_config->init();
    g_windMgr->init();
    // g_connMgr->init();
    // g_btnMgr->init();
    // g_tuyaMgr->init();
    return app.exec();
}

