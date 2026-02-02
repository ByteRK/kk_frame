/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-02 16:22:16
 * @LastEditTime: 2026-02-02 16:23:41
 * @FilePath: /kk_frame/src/comm/wifi/wifi_set.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIFI_SET_H__
#define __WIFI_SET_H__

#include "wifi_adapter.h"

class SetWifi {
private:
    /* 自动连接网络 */
    static bool mAutoConnect;

public:
    SetWifi();
    ~SetWifi();   

    static void autoConnect();
    static void setAuto(bool flag);
};

#endif // !__WIFI_SET_H__
