/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2024-08-22 18:22:56
 * @FilePath: /kk_frame/src/project/config_mgr.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef __CONFIG_MGR_H__
#define __CONFIG_MGR_H__

#include <core/handler.h>
#include <core/uieventsource.h>
#include <core/preferences.h>
#include "common.h"
#include "hv_defualt_config.h"

#define g_config configMgr::ins()

class configMgr :public MessageHandler {
private:
    Message  mMsg;
    Looper*  mLooper;
    int      mUpdates;
    uint64_t mNextBakTick;

    cdroid::Preferences mConfig; // 配置文件读写

private:
protected:
    configMgr();
    ~configMgr();

    void update();
public:
    static configMgr* ins() {
        static configMgr stIns;
        return &stIns;
    }
    void handleMessage(Message& message)override;

    void init();
};


#endif // __CONFIG_MGR_H__