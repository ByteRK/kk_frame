/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_wifi.h
 * @Description: WIFI检测
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_WIFI_H__
#define __FACTORY_WIFI_H__

#include "page_factory.h"
#include "wifi_mgr.h"

/// @brief WIFI检测
class FactoryWifi : public PageFactory::FactoryItem,
    public WifiMgr::WiFiListener {
private:
    TextView* mWifiList{ nullptr };
    TextView* mWifiConnInfo{ nullptr };
    TextView* mWifiRefresh{ nullptr };
    TextView* mWifiSwitch{ nullptr };

public:
    FactoryWifi(ViewGroup* container, PageFactory* factory);
    ~FactoryWifi() override;

protected:
    void onShow() override;
    void onStateChanged() override;
    void onScanResult() override;

private:
    void updateWifiInfo();
};

#endif // __FACTORY_WIFI_H__
