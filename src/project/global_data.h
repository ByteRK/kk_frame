/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:53:50
 * @LastEditTime: 2024-08-23 09:35:09
 * @FilePath: /kk_frame/src/project/global_data.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef _GLOBAL_DATA_H_
#define _GLOBAL_DATA_H_

#include "struct.h"

#include <core/uieventsource.h>
#include <core/preferences.h>

#define g_data globalData::ins()

class globalData :public MessageHandler {
public:
    enum {
        WIFI_NULL = 0,
        WIFI_1,
        WIFI_2,
        WIFI_3,
        WIFI_4,
        WIFI_ERROR,
    };

    uint8_t     mNetWork = WIFI_NULL;     // 网络状态
    uint8_t     mNetWorkDetail = 0;       // 网络详细状态

private:
    globalData();
    ~globalData();
private:
    enum {
        MSG_SAVE,
    };

    Looper*          mLooper;        
    bool             mHaveChange;
    bool             mNeedSaveBak;
    uint64_t         mNextBakTick;


    Message          mSaveMsg;
    uint64_t         mPowerOnTime;  // 启动时间
public:
    static globalData* ins() {
        static globalData s_globalData;
        return &s_globalData;
    }
    void handleMessage(Message& message)override;

    void init();

    uint64_t getPowerOnTime();
private:
    /*模式信息*/
    void loadMode();
    bool loadFromFile();
    bool saveMode(bool isBak = false);
};

#endif // _GLOBAL_DATA_H_