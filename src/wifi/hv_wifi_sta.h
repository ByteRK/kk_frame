#ifndef __HV_WIFI_STA_H__
#define __HV_WIFI_STA_H__

#include "stdint.h"
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif
#include "wpa_ctrl.h"
#ifdef __cplusplus
}
#endif

class WifiSta {
public:
    enum CONENCT_STATUS_E {
        E_DISCONNECT = 0,
        E_STA_CONENCTING,
        E_STA_CONENCTINGED,
        E_CONENCT_STA_DHCP,
        E_CONENCT_STA_GUARD, // 这里才是真正得连接完成wifi
        E_STA_AP_NOT_FOUND,
        E_ERROR_PASSWORD,
        E_CONNECT_SUCCESS,  // 连接成功
    };

    struct WIFI_ITEM_S {
        std::string ssid;
        std::string mac;
        int         quality;        // 质量
        int         signalLevel;    // 信号强度
        int         signal; // 信号 1-4
        bool        isKey;  // 是否有密码
        int         rssi;   // 负数
    };

public:
    typedef std::function<void()> OnConenctListener;
    static WifiSta               *ins() {
        static WifiSta sta;
        return &sta;
    }
    ~WifiSta();
    int32_t          scan(std::list<WIFI_ITEM_S> &wifilist);
    int32_t          connect(const std::string &name, const std::string &key);
    CONENCT_STATUS_E get_status();
    void             disconnect();
    void             disEnableWifi();
    void             enableWifi();

private:
    class ThreadInfo {
    public:
        ThreadInfo(const std::string &inName, const std::string &inKey) {
            conn_status  = E_STA_CONENCTING;
            running      = true;
            softResetCnt = 0;
            ssid         = inName;
            key          = inKey;
        }
        ~ThreadInfo() {  }

    public:
        CONENCT_STATUS_E conn_status;
        bool             running;
        int              softResetCnt;
        std::string      ssid;
        std::string      key;
    };

    CONENCT_STATUS_E handle_result(CONENCT_STATUS_E old, struct wifi_status_result_t &result);
    void             conenct_run(ThreadInfo *lpInfo);
    void             connect_wifimanager_run(ThreadInfo *lpInfo);

protected:
    WifiSta();

    int executeCommand(const std::string& command);
    std::string executeCommandWithOutput(const std::string& command);
    bool checkConnectionStatus(const std::string& expectedSSID = "");
private:
    std::mutex  mMutex;
    ThreadInfo *mCurThreadInfo;
    CONENCT_STATUS_E mWifiConnStatus;
};

void parseWifiScan(std::list<WifiSta::WIFI_ITEM_S> &wifiCells, const std::string &scanString);
void parseWifiScan2(std::list<WifiSta::WIFI_ITEM_S> &wifiCells, const std::string &scanString);

#endif