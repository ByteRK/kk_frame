/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-02 16:22:43
 * @LastEditTime: 2026-02-06 09:16:29
 * @FilePath: /kk_frame/src/comm/wifi/wifi_set.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wifi_set.h"

#if ENABLED(WIFI) || defined(__VSCODE__)

#include "config_mgr.h"
#include "global_data.h"

#define WIFI_CONNECT_TIMEOUT 15000 // wifi连接超时 */

////////////////////////////////////////////////////////////////////////////////////////////////////////
// wifi window
bool SetWifi::mAutoConnect = true;

SetWifi::SetWifi() {
    mAutoConnect = false;
}

SetWifi::~SetWifi() {
    WIFIAdapter::instance()->cancel();
    mAutoConnect = true;
}

void SetWifi::autoConnect() {
    if (!g_config->getWifi()) return;
    if (g_data->mNetOK) return;
    if (!mAutoConnect) return;

    // 搜索
    std::string name = g_config->getWifiSSID(), passwd = g_config->getWifiPassword();
    if (name.empty()) return;

    size_t handle = WifiSta::ins()->connect(name, passwd);

    int checkConnect = 0;
    while (mAutoConnect) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        checkConnect++;
        if (WifiSta::ins()->get_status() == WifiSta::E_CONENCT_STA_GUARD) {
            LOGI("connect ok. name=%s", name.c_str());
            break;
        }

        if (checkConnect >= 50) {
            LOGW("connect time out!!! name=%s passwd=%s", name.c_str(), passwd.c_str());
            break;
        }
    }

    WifiSta::ins()->exit(handle);
}

void SetWifi::setAuto(bool flag) {
    mAutoConnect = flag;
}

#endif // !ENABLED(WIFI)