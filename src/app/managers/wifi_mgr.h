/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-27 17:11:43
 * @LastEditTime: 2026-02-28 18:12:35
 * @FilePath: /kk_frame/src/app/managers/wifi_mgr.h
 * @Description: WIFI 管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIFI_MGR_H__
#define __WIFI_MGR_H__

#include "wifi_hal.h"
#include "template/singleton.h"
#include "class/auto_save.h"
#include <atomic>
#include <core/looper.h>
#include <core/preferences.h>
#include <set>

#define g_wifi WifiMgr::instance()

class WifiMgr :public Singleton<WifiMgr>,
    public cdroid::EventHandler, public AutoSaveItem {
    friend class Singleton<WifiMgr>;
public:
    class WiFiListener {
    public:
        virtual void onStateChanged() { };
        virtual void onScanResult() { };
    };

protected:
    WifiMgr();

public:
    ~WifiMgr();
    void           init();
    void           reset();
    void           addListener(WiFiListener* listener);
    void           removeListener(WiFiListener* listener);

    WifiHal::State getState();
    void           getAps(std::vector<WifiHal::ApInfo>& aps);

    bool           scan();
    bool           connect(const std::string& ssid, const std::string& psk);
    bool           disconnect();

    void           setSwitch(bool enable);
    bool           getSwitch();
    void           getConnectInfo(std::string& ssid, std::string& psk);
    bool           getConnectedAp(WifiHal::ApInfo& ap);

protected:
    int            checkEvents() override;
    int            handleEvents() override;
    bool           save(bool isBackup = false) override;
    bool           haveChange() override;

private:
    void           load();
    void           onStateChanged(WifiHal::State state, const std::string& reason);
    void           onScanResult(const std::vector<WifiHal::ApInfo>& aps);

    void           updateResultAfterConnected();

private:
    cdroid::Preferences           mOption;
    bool                          mAutoConnect{ false };

    WifiHal*                      mWifiHal{ nullptr };
    int64_t                       mNextEventTime{ 0 };
    std::set<WiFiListener*>       mListeners;

    std::vector<WifiHal::ApInfo>  mAps;
    std::mutex                    mApsMutex;
    std::atomic<bool>             mApsChanged{ false };

    WifiHal::State                mState;
    std::mutex                    mStateMutex;
    std::atomic<bool>             mStateChanged{ false };
};

#endif // !__WIFI_MGR_H__
