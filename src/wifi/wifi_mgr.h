// /*
//  * @Author: Ricken
//  * @Email: me@ricken.cn
//  * @Date: 2025-10-28 15:34:57
//  * @LastEditTime: 2025-10-28 17:29:45
//  * @FilePath: /ylw_oven/src/wifi/wifi_mgr.h
//  * @Description: WIFI管理
//  * @BugList:
//  *
//  * Copyright (c) 2025 by Ricken, All Rights Reserved.
//  *
// **/

// #ifndef _WIFI_MGR_H_
// #define _WIFI_MGR_H_

// #include "common.h"
// #include <core/looper.h>

// #define g_wifi WifiMgr::ins()

// // WIFI列表
// typedef struct {
//     uint8_t     locked : 1;      // 需要密码
//     int         quality;         // 质量
//     int         signalLevel;     // 信号强度
//     int         level;           // 信号强度
//     uint8_t     conn_status : 2; // 连接状态
//     std::string name;            // 名称
// } WIFIInfo;

// // WIFI 连接信息
// typedef struct {
//     uint8_t     locked : 1;      // 需要密码
//     std::string name;            // 名称
//     std::string key;
// } WIFIConnectData;

// // WIFI状态
// enum WIFIStatus {
//     WIFI_CONNECTING,        // 连接中
//     WIFI_CONNECTED,         // 已连接
//     WIFI_DISCONNECTED,      // 未连接
// };

// class WifiMgr :public cdroid::MessageHandler {
// private:
//     static WifiMgr* instance;
//     WifiMgr(); // 私有构造函数
//     WifiMgr(const WifiMgr&) = delete; // 禁止拷贝构造
//     WifiMgr& operator=(const WifiMgr&) = delete; // 禁止赋值操作
// public:
//     static WifiMgr* ins() {
//         if (instance == nullptr)
//             instance = new WifiMgr();
//         return instance;
//     }
//     ~WifiMgr();
//     void init();

//     void handleMessage(cdroid::Message& message)override;
// private:
//     enum {
//         MSG_DELAY_CONN,         // 延迟 连接wifi
//         MSG_DELAY_DISCONN,      // 延迟 断开连接wifi
//     };
// public:
//     static WIFIConnectData sConnData;            // WIFI连接信息(非UI线程使用)
//     static std::vector<WIFIInfo> sWifiScanData;  // WIFI扫描列表(非UI线程使用)
// private:
//     WIFIConnectData       mConnData;             // WIFI连接信息
//     std::vector<WIFIInfo> mWifiScanData;         // WIFI扫描列表

//     bool mIsStartScan;                           // 是否开始扫描
//     WIFIStatus mConnStatus;                      // 连接状态

// public:
//     void set(bool enable);
//     WIFIStatus status();
//     void scan();
//     void conn(WIFIConnectData connWifiData, uint64_t delay);
//     void disConn();

// protected:
//     void scanEnd();
//     void setStatus(WIFIStatus status);
// };

// #endif
