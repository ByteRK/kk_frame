/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-02 19:41:33
 * @LastEditTime: 2026-02-06 09:15:56
 * @FilePath: /kk_frame/src/comm/wifi/wifi_sta.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIFI_STA_H__
#define __WIFI_STA_H__

#include "common.h"
#if ENABLED(WIFI) || defined(__VSCODE__)

#include <stdio.h>
#include <stdint.h>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <set>

#ifdef __cplusplus
extern "C" {
#endif
#include <wpa_ctrl.h>
#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////
class WifiSta {
public:
    enum CONENCT_STATUS_E {
        E_ERROR_UNKNOW = 0,
        E_STA_CONENCTING,
        E_STA_CONENCTINGED,
        E_CONENCT_STA_DHCP,
        E_CONENCT_STA_GUARD, // 这里才是真正得连接完成wifi
        E_STA_AP_NOT_FOUND,
        E_ERROR_PASSWORD,    /* 4WAY_HANDSHAKE */
        E_DISCONENCT_SUCCESS,
    };

    struct WIFI_ITEM_S {
        std::string mac;     // mac地址
        std::string ssid;    // 名称
        bool        encrypt; // 是否加密
        int         signal;    // 信号等级 x/100
        WIFI_ITEM_S(){
            reset();
        }
        void reset(){
            mac.clear();
            ssid.clear();
            encrypt = true;
            signal = 0;
        }
    };

public:
    typedef std::function<void()> OnConenctListener;
    static WifiSta               *ins() {
        static WifiSta sta;
        return &sta;
    }
    ~WifiSta();
    int32_t          scan(std::list<WIFI_ITEM_S> &wifilist);                      // 扫描wifi
    size_t           connect(const std::string &name, const std::string &key);    // 开始连接
    CONENCT_STATUS_E get_status();                                                // 获取状态
    void             exit(size_t handle = 0);                                     // 退出连接
    void             down();                                                      // 关闭网络
    void             up();                                                        // 启用网络

private:
    class ThreadInfo {
    public:
        ThreadInfo(const std::string &inName, const std::string &inKey) {
            th           = 0;
            conn_status  = E_STA_CONENCTING;
            running      = true;
            softResetCnt = 0;
            ssid         = inName;
            key          = inKey;
        }
        ~ThreadInfo() {
            if (th) {
                delete th;
                th = 0;
            }
        }

    public:
        std::thread     *th;
        CONENCT_STATUS_E conn_status;
        bool             running;
        int              softResetCnt;
        std::string      ssid;
        std::string      key;
    };

    CONENCT_STATUS_E handle_result(CONENCT_STATUS_E old, struct wifi_status_result_t &result);
    void             connect_run(ThreadInfo *lpInfo);

protected:
    WifiSta();

private:
    std::mutex  mMutex;    
    ThreadInfo *mCurThreadInfo;
    std::set<size_t> mHandles;
};

#endif // !ENABLED(WIFI)

#endif // !__WIFI_STA_H__
