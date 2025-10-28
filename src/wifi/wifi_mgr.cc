// /*
//  * @Author: Ricken
//  * @Email: me@ricken.cn
//  * @Date: 2025-10-28 15:35:06
//  * @LastEditTime: 2025-10-28 17:48:34
//  * @FilePath: /ylw_oven/src/wifi/wifi_mgr.cc
//  * @Description: WIFI管理
//  * @BugList:
//  *
//  * Copyright (c) 2025 by Ricken, All Rights Reserved.
//  *
//  */

// #include "wifi_mgr.h"
// #include "config_mgr.h"

// WifiMgr::WifiMgr() {
// }

// WifiMgr::~WifiMgr() {
//     delete instance;
//     instance = nullptr;
// }

// void WifiMgr::init() {
//     mIsStartScan = false;
//     mConnStatus = WIFI_DISCONNECTED;
// }

// void WifiMgr::handleMessage(cdroid::Message& message) {
//     switch (message.what) {
//     case MSG_DELAY_CONN: {
//         if (g_config->getWifiSwitch()) {
//             mConnStatus = WIFI_CONNECTING;
//             WifiSta::ins()->disconnect();
//             Activity_header::ins()->changeState(PRO_STATE_NET_NONE);
//             ThreadPool::ins()->add(new WIFIConnectThread, &mConnData, true);
//         }
//     }   break;
//     case MSG_DELAY_DISCONN: {

//         if (mConnStatus == WIFI_CONNECTING && WifiSta::ins()->get_status() == WifiSta::E_STA_CONENCTING) disConnWifi();
//     }   break;
//     default:
//         break;
//     }

//     if (message.what == MSG_DELAY_CONN) {
//     } else if (message.what == MSG_DELAY_DISCONN) {
//     }
// }


















// int WIFIScanThread::onTask(void* data) {
//     std::vector<WIFIAdapterData>* dat = (std::vector<WIFIAdapterData> *)data;
//     dat->clear();
//     std::list<WifiSta::WIFI_ITEM_S> wifiList;
//     WifiSta::ins()->scan(wifiList);
//     std::string wifiName, wifiPasswd;
//     g_objConf->getWifiInfo(wifiName, wifiPasswd);

//     for (WifiSta::WIFI_ITEM_S& item : wifiList) {
//         if (item.signal <= 0) continue;
//         WIFIAdapterData wf_data;
//         wf_data.locked = item.isKey;
//         wf_data.level = item.signal;
//         wf_data.conn_status = (wifiName == item.ssid) ? WIFI_CONNECTED : WIFI_DISCONNECTED;
//         wf_data.name = item.ssid;
//         wf_data.quality = item.quality;
//         wf_data.signalLevel = item.signalLevel;
//         if (((wf_data.name == wifiName) || (wf_data.name == connWifiData.name)) && ((!g_appData.netOk && WIFIMgr::ins()->connStatus() == WIFI_CONNECTING) || g_appData.netOk)) {
//             g_appData.netStatus = PRO_STATE_NET_NONE + wf_data.level;
//             LOGI_IF(wifiName == wf_data.name, " /*   g_appData.netStatus  */ ");
//         } else if ((wf_data.name == wifiName) && (WIFIMgr::ins()->connStatus() == WIFI_DISCONNECTED) && !g_appData.netOk) {
//             connWifiData.name = wifiName;
//             connWifiData.key = wifiPasswd;
//             g_appData.netStatus = PRO_STATE_NET_NONE + wf_data.level;
//             LOGE("/*  connWifiData.name = %s  connWifiData.key = %s  */ ", connWifiData.name.c_str(), connWifiData.key.c_str());
//             WIFIMgr::ins()->delayConnWifi(connWifiData, 1000);
//         } else {
//             dat->push_back(wf_data);
//             LOGI_IF(wifiName == wf_data.name, " /*  dat->push_back(wf_data)  */");
//         }

//     }
//     return 0;
// }

// void WIFIScanThread::onMain(void* data) {
//     static int count = 0;
//     if (g_appData.netSwitch) {
//         std::vector<WIFIAdapterData> wifiScanList = *(std::vector<WIFIAdapterData> *)data;
//         if ((wifiScanList.size() == 0) && (WIFIMgr::ins()->connStatus() == WIFI_CONNECTED)) {
//             WIFIMgr::ins()->scanEnd();
//             Activity_header::ins()->changeState((State)g_appData.netStatus);
//             if ((Activity_header::ins()->getActivityPageType() == ACTIVITY_SETUP) && (count < 5)) {
//                 count++;
//                 WIFIMgr::ins()->scanWifi();
//             } else {
//                 count = 0;
//             }
//             return;
//         }
//         sWifiData = wifiScanList;
//         g_objWindMgr->delayAppCallback(CS_WIFI_ADAPTER_NOTIFI);
//         Activity_header::ins()->changeState((State)g_appData.netStatus);
//         count = 0;
//         LOGE("scan wifi complet  sWifiData.size() = %d", sWifiData.size());
//     } else {
//         sWifiData.clear();
//         LOGE("scan wifi fail!");
//     }
//     WIFIMgr::ins()->scanEnd();
// }

// int WIFIConnectThread::onTask(void* data) {
//     WIFIConnectData* dat = (WIFIConnectData*)data;
//     WifiSta::ins()->connect(dat->name, dat->key);
//     return 0;
// }

// void WIFIConnectThread::onMain(void* data) {
//     WIFIConnectData* dat = (WIFIConnectData*)data;
//     connWifiData = {};
//     if (g_appData.netSwitch) {
//         LOGE("connect wifi complet WifiSta::ins()->get_status() = %d", WifiSta::ins()->get_status());
//         if (WifiSta::ins()->get_status() == WifiSta::E_CONNECT_SUCCESS) {
//             g_appData.netOk = true;
//             g_objConf->setWifiInfo(dat->name, dat->key);
//             for (auto it = sWifiData.begin(); it != sWifiData.end(); it++) {
//                 if (it->name == dat->name) {
//                     g_appData.netStatus = it->level + PRO_STATE_NET_NONE;
//                     sWifiData.erase(it);
//                     break;
//                 }
//             }
//             WIFIMgr::ins()->setConnStatus(WIFI_CONNECTED);
//             g_objWindMgr->AppCallback(CS_WIFI_CONNECT);
//             g_objWindMgr->AppCallback(CS_NETWORK_CHANGE);
//             g_appData.nextUpdateTime = SystemClock::uptimeMillis() + 3 * 1000;
//         } else {
//             g_appData.netStatus = PRO_STATE_NET_NONE;
//             WIFIMgr::ins()->setConnStatus(WIFI_DISCONNECTED);
//             WIFIMgr::ins()->scanWifi();
//         }
//     } else {
//         LOGE("g_appData.netSwitch is close !");
//         g_appData.netStatus = PRO_STATE_NET_NONE;
//         WifiSta::ins()->disconnect();
//     }
//     Activity_header::ins()->changeState((State)g_appData.netStatus);
// }




// /// @brief 设置WIFI开关
// /// @param enable 
// void WifiMgr::set(bool enable) {
//     if (enable) {
//         WifiSta::ins()->enableWifi();
//     } else {
//         WifiSta::ins()->disEnableWifi();
//         WifiSta::ins()->disconnect();
//         mIsConnStatus = WIFI_DISCONNECTED;
//     }
// }

// /// @brief 获取WIFI状态
// /// @return 
// WIFIStatus WifiMgr::status() {
//     return mConnStatus;
// }

// /// @brief 扫描WIFI
// void WifiMgr::scan() {
//     if (g_appData.netSwitch && !mIsStartScan) {
//         // system("ifconfig wlan0 up");
//         mIsStartScan = true;
//         ThreadPool::ins()->add(new WIFIScanThread, &sWifiScanData, true);
//     }
// }

// /// @brief 连接WIFI
// /// @param connWifiData 
// /// @param delay 
// void WifiMgr::conn(WIFIConnectData connWifiData, uint64_t delay) {
//     if (g_appData.netSwitch) {
//         mConnData = connWifiData;
//         Message connMSG, disconnMSG;
//         connMSG.what = MSG_DELAY_CONN;
//         disconnMSG.what = MSG_DELAY_DISCONN;

//         Looper::getMainLooper()->removeMessages(this);
//         Looper::getMainLooper()->sendMessageDelayed(delay, this, connMSG);
//         Looper::getMainLooper()->sendMessageDelayed(delay + 30 * 1000, this, disconnMSG);
//     }
// }

// /// @brief 断开WIFI
// void WifiMgr::disConn() {
//     if (mConnStatus != WIFI_CONNECTING) {
//         g_config->setWifiSsid("");
//         g_config->setWifiPassword("");
//     }
//     connWifiData = {};
//     g_appData.netOk = false;
//     g_appData.netStatus = PRO_STATE_NET_NONE;
//     mConnStatus = WIFI_DISCONNECTED;
//     WifiSta::ins()->disconnect();
//     scan();
// }

// /// @brief 扫描结束
// void WifiMgr::scanEnd() {
//     mIsStartScan = false;
// }

// /// @brief 更新WIFI状态
// /// @param status 
// void WifiMgr::setStatus(WIFIStatus status) {
//     mConnStatus = status;
// }
