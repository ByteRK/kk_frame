

#include <cstdio>
#include <iostream>
#include <regex>
#include <sys/prctl.h>
#include <unistd.h>
#include <core/textutils.h>
#include <hv_series_conf.h>
#include <hv_icmp.h>
#include <hv_net.h>
#include <comm_func.h>

#include "hv_wifi_sta.h"
#include <cdlog.h>

using namespace std;

#define WPA_INIT_CMD          "echo service>tmp/wpa.cmd && sleep 1"
#define WPA_WORK_DIR          "/userdata/app"
#define WPA_SUPPLICANT_SCRIPT WPA_WORK_DIR "/wpa_supplicant.conf"

#if defined(PRODUCT_X64)
#define HV_POPEN(__cmd)
#else
#define HV_POPEN(__cmd...)                                                                                             \
    do {                                                                                                               \
        char __result[256] = {0};                                                                                      \
        snprintf(__result, sizeof(__result), ##__cmd);                                                                 \
        system(__result);                                                                                              \
        printf("[wifi]%s\n", __result);                                                                              \
    } while (0)
#endif

static void parseWifiScan(std::list<WifiSta::WIFI_ITEM_S> &wifiCells, const std::string &scanString) {
    std::istringstream   iss(scanString);
    std::string          line, val;
    WifiSta::WIFI_ITEM_S cell;

    while (std::getline(iss, line)) {
#if 1
        // d2:ed:07:51:53:75       2457    -56     [WPA2-PSK-CCMP][ESS]       xiaomi-hook
        // c2:3d:c6:5f:74:bf       2437    -30     [WPA2-PSK-CCMP][ESS]    WIFI \xe5\xa4\x8f\\x0a
        if (line.find("[ESS]") == std::string::npos) continue;

        int         column = 0;
        const char *p, *pl = 0;
        for (p = line.c_str(); *p; p++) {
            if (*p == ' ' || *p == '\t') {
                if (pl) {
                    switch (column) {
                    case 0: /* bssid */ cell.mac.assign(pl, p - pl); break;
                    case 1: /* frequency */ break;
                    case 2: /* signal level */ cell.signal = atoi(pl); break;
                    case 3: /* flags */ cell.encrypt = strstr(pl, "[WPA") != NULL; break;
                    case 4: /* ssid */ cell.ssid = convert_to_unicode(pl); break;
                    default: break;
                    }
                    column++;
                    pl = 0;
                }
                continue;
            }
            if (!pl) pl = p;
        }
        if (pl && *pl && cell.ssid.empty()) {
            cell.ssid = convert_to_unicode(pl);
        }
#else
        // bss[09]: bssid=22:8e:b2:4f:48:2f  ssid=   xiaomi-hook     channel=1(freq=2412)  rssi=-50  sec=WPA2_PSK
        if (line.find("bssid=") == std::string::npos) continue;
        cell.mac     = get_mid_str(line, " bssid=", "  ssid=");
        cell.ssid    = get_mid_str(line, "  ssid=", "  channel=");
        cell.signal  = std::atoi(get_mid_str(line, "  rssi=", "  sec=").c_str());
        cell.encrypt = get_mid_str(line, "  sec=", "") != "NONE";
#endif
        if (!cell.mac.empty() && !cell.ssid.empty()) {
            /* 过滤重名，保留信号最好的 */
            bool ssid_exists = false;
            for (WifiSta::WIFI_ITEM_S &item : wifiCells) {
                if (item.ssid == cell.ssid) {
                    if (cell.signal > item.signal) item.signal = cell.signal;
                    ssid_exists = true;
                    break;
                }
            }
            if (!ssid_exists) wifiCells.push_back(cell);
        }
        cell.reset();
    }
}

/// @brief
/// @param ucSSID
/// @param ucKey
/// @return
static int __save_conf(const std::string &ssid, const std::string &key) {
    FILE *pFile = NULL;

    pFile = fopen(WPA_SUPPLICANT_SCRIPT, "wt");
    if (pFile == nullptr) {
        perror("open file wpa config error:");
        return -1;
    }

    if (pFile != NULL) {
        fprintf(pFile, "ctrl_interface=/var/run/wpa_supplicant\n");
        fprintf(pFile, "update_config=1\n");
        fprintf(pFile, "ap_scan=1\n");
        fprintf(pFile, "network={\n");
        fprintf(pFile, "\tssid=\"%s\"\n", ssid.c_str());
        fprintf(pFile, "\tscan_ssid=1\n");
        // 判断密码有无来决定wifi配置文件加密方式 */
        if (key.length() == 0){
            LOGI("[%s] no passwd!!", ssid.c_str());
            fprintf(pFile, "\tkey_mgmt=NONE\n");
        } else {
            LOGI("[%s]need passwd!! default WPA-PSK", ssid.c_str());
            fprintf(pFile, "\tkey_mgmt=WPA-PSK WPA-EAP IEEE8021X NONE\n");
            fprintf(pFile, "\tpairwise=TKIP CCMP\n");
            // fprintf(pFile, "\tgroup=CCMP TKIP WEP104 WEP40\n");
            fprintf(pFile, "\tpsk=\"%s\"\n", key.c_str());
        }
        fprintf(pFile, "}");
        fclose(pFile);
        pFile = NULL;
    } else {
        LOGE("open %s Error!!!", WPA_SUPPLICANT_SCRIPT);
        return -1;
    }
    return 0;
}

/// @brief 重新启动wpa进程
static void __kill_wpa() {
    HV_POPEN("killall -9 udhcpc");
    HV_POPEN("ifconfig " HV_WLAN_NAME " down");
    HV_POPEN("ifconfig " HV_WLAN_NAME " up");
    HV_POPEN("killall -9 wpa_supplicant");
}

static void __start_wpa_srv() {
    HV_POPEN("[ -n \"$(ifconfig | grep wlan0)\" ] && [ -z \"$(pidof wpa_supplicant)\" ] && " WPA_INIT_CMD);
}

/*****************************************************/
WifiSta::WifiSta(/* args */) {
    mCurThreadInfo = 0;
#if HV_FUNCTION_WIFI == 1
    wpa_ctrl_run_set_path("/var/run");
#endif
}

WifiSta::~WifiSta() {
    mMutex.lock();
    if (mCurThreadInfo) { mCurThreadInfo->running = false; }
    mMutex.unlock();
}

/// @brief
/// @param wifi_lsit
/// @return
int32_t WifiSta::scan(std::list<WIFI_ITEM_S> &wifilist) {
    __start_wpa_srv();
#if HV_FUNCTION_WIFI == 1
    int scane_count = 0;
    do {
        sysCommand("wpa_cli -i " HV_WLAN_NAME " scan");
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // 1.5s wait scan    
        std::string bufStr = sysCommand("wpa_cli -i " HV_WLAN_NAME " scan_result");
        parseWifiScan(wifilist, bufStr);
    }while (wifilist.empty() && ++scane_count < 3);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    for (uint8_t i = 0; i < 10; i++) {
        WIFI_ITEM_S item;
        item.ssid    = "test wifi " + std::to_string(i);
        item.mac     = "88:88:88:88:" + std::to_string(i);
        item.encrypt = __rand_value(0, 1);
        item.signal  = __rand_value(-100, 0);
        wifilist.push_back(item);
    }
#endif
    // 信号排序
    int i = 0;
    std::cout << "---------------------------------" << std::endl;
    wifilist.sort([](const WIFI_ITEM_S &a, const WIFI_ITEM_S &b) -> bool { return a.signal > b.signal; });
    for (const auto &cell : wifilist) {
        std::cout << (++i) << ". - Address: " << cell.mac << std::endl;
        std::cout << "  ESSID: " << cell.ssid << std::endl;
        std::cout << "  Lock: " << cell.encrypt << std::endl;
        std::cout << "  Signal: " << cell.signal << std::endl;
    }
    std::cout << "---------------------------------" << std::endl;
    return 0;
}

/// @brief 链接
/// @param name
/// @param key
/// @return
size_t WifiSta::connect(const std::string &name, const std::string &key) {
    size_t handle = 0;
#if HV_FUNCTION_WIFI == 1
    // 保存配置文件。连接wifi
    __save_conf(name, key);
#endif

    mMutex.lock();
    if (mCurThreadInfo) mCurThreadInfo->running = false;
    mCurThreadInfo = new ThreadInfo(name, key);
    mCurThreadInfo->th = new std::thread(std::bind(&WifiSta::connect_run, this, std::placeholders::_1), mCurThreadInfo);
#if HV_FUNCTION_WIFI == 0
    mCurThreadInfo->conn_status = E_CONENCT_STA_GUARD;
#endif
    mCurThreadInfo->th->detach();
    handle = reinterpret_cast<size_t>(mCurThreadInfo);
    mHandles.insert(handle);
    mMutex.unlock();

    return handle;
}

/**
 * 处理 WPA 的控制状态。
 *
 */
WifiSta::CONENCT_STATUS_E WifiSta::handle_result(CONENCT_STATUS_E old, struct wifi_status_result_t &result) {
    CONENCT_STATUS_E state = old;
    printf("wifi sta state:%s\n", result.wpa_state);
    if (strcmp(result.wpa_state, "COMPLETED") == 0) {
        state = E_STA_CONENCTINGED;
    } else if (strcmp(result.wpa_state, "SCANNING") == 0) {
        // 校验超时。
        state = E_STA_CONENCTING;
    } else if (strcmp(result.wpa_state, "4WAY_HANDSHAKE") == 0){
        state = E_ERROR_PASSWORD;
    } else {
    }
    return state;
}

//
void WifiSta::connect_run(ThreadInfo *lpInfo) {
    LOGI("wifi sta thread start. ssid=%s", lpInfo->ssid.c_str());

#if HV_FUNCTION_WIFI == 1
    char szGateway[16] = {0};
    prctl(PR_SET_NAME, __func__);
    uint8_t pingCnt = 0;

GOTO_CONENNCTION_REFUSED:
    if (lpInfo->softResetCnt > 5) {
        while (lpInfo->running) {
            LOGW("%s abnormal. May require a reboot; status=%d", HV_WLAN_NAME, lpInfo->conn_status);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        mMutex.lock();
        if (lpInfo == mCurThreadInfo) mCurThreadInfo = nullptr;
        delete lpInfo;
        mMutex.unlock();
        return;
    }

    lpInfo->softResetCnt++;

    __kill_wpa();

    /* 启动脚本链接wifi 在程序内启动会导致rkadk启动失败 */
    // HV_POPEN("wpa_supplicant -B -Dnl80211 -i" HV_WLAN_NAME " -c" WPA_SUPPLICANT_SCRIPT);
    HV_POPEN("echo connect>tmp/wpa.cmd");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int last_status     = E_ERROR_UNKNOW;
    lpInfo->conn_status = E_STA_CONENCTING;
    struct timespec stTime;
    clock_gettime(CLOCK_MONOTONIC, &stTime);
    uint32_t old_timecount = stTime.tv_sec;

    pingCnt = 0;
    while (lpInfo->running) {
        clock_gettime(CLOCK_MONOTONIC, &stTime);

        if (last_status != lpInfo->conn_status) {
            LOGI("status changed. old=%d new=%d", last_status, lpInfo->conn_status);
            last_status = lpInfo->conn_status;
        }

        switch (lpInfo->conn_status) {
        case E_STA_CONENCTING: {
            // 查询状态 */
            struct wifi_status_result_t result;
            if (wpa_ctrl_sta_status(&result) == 0) {
                lpInfo->conn_status = handle_result(lpInfo->conn_status, result);
                old_timecount       = stTime.tv_sec;
            }

            if (lpInfo->conn_status == E_ERROR_PASSWORD) {
                lpInfo->softResetCnt = 5;
                goto GOTO_CONENNCTION_REFUSED;
            } else if (stTime.tv_sec - old_timecount > 60) {
                LOGW("connection timed out");
                int rssi;
                if (wpa_ctrl_sta_get_near_ap_list(lpInfo->ssid.c_str(), &rssi) == 0) {
                    lpInfo->conn_status = E_STA_CONENCTING;
                } else {
                    LOGW("ap not find");
                }
                goto GOTO_CONENNCTION_REFUSED;
            }
        } break;
        // 连接完成 分配或设置IP */
        case E_STA_CONENCTINGED: {
            HV_POPEN("udhcpc -i " HV_WLAN_NAME " -n -q");
            lpInfo->conn_status = E_CONENCT_STA_DHCP;
        } break;
        // 检查IP 地址 和 MAC 不为空 */
        case E_CONENCT_STA_DHCP: {
            char szIP[16];
            char szMac[32];
            HV_NET_GetIpAddr(HV_WLAN_NAME, szIP);
            HV_NET_GetMacAddr(HV_WLAN_NAME, NULL, szMac);
            if (szIP[0] == '\0') {
                LOGW("no correct ip!!!! softreset");
                goto GOTO_CONENNCTION_REFUSED;
            }
            if (szMac[0] == '\0') {
                LOGW("no correct mac!!!! softreset");
                goto GOTO_CONENNCTION_REFUSED;
            }
            LOGI("ip=%s mac=%s", szIP, szMac);
            lpInfo->conn_status = E_CONENCT_STA_GUARD;
        } break;
        case E_CONENCT_STA_GUARD: {
            // 还没获取到网关 */
            if (szGateway[0] == 0) {
                HV_NET_GetGateway(HV_WLAN_NAME, szGateway, sizeof(szGateway));
                if (szGateway[0] == 0) {
                    LOGW("not corect gatewat!!! softreset");
                    goto GOTO_CONENNCTION_REFUSED;
                }
                LOGI("gateway=%s", szGateway);
            }
            if (stTime.tv_sec - old_timecount > 5) {
                old_timecount = stTime.tv_sec;

                if (ping_host_ip(szGateway, 1, 0) == 0) {
                    pingCnt = 0;
                } else {
                    pingCnt++;
                    LOGW("NetWork ping failed cnt:%d %s", pingCnt, szGateway);
                }
            }
            if (pingCnt > 10) {
                // 网卡异常
                if (HV_NET_CheckNicStatusUp(HV_WLAN_NAME) != 0) {
                    LOGW("Network card abnormality");
                    goto GOTO_CONENNCTION_REFUSED;
                }

                // 网络连接异常
                else if (HV_NET_CheckNicStatus(HV_WLAN_NAME) != 0) {
                    // 重新连接wifi
                    LOGW("Network link abnormality");
                    goto GOTO_CONENNCTION_REFUSED;
                }
            }
        } break;
        default: break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
#else
    while (lpInfo->running) std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    mMutex.lock();
    if (lpInfo == mCurThreadInfo) mCurThreadInfo = nullptr;
    delete lpInfo;
    mHandles.erase((size_t)lpInfo);
    mMutex.unlock();

    LOGI("wifi sta thread exit.");
}

WifiSta::CONENCT_STATUS_E WifiSta::get_status() {
    WifiSta::CONENCT_STATUS_E status = E_ERROR_UNKNOW;
    mMutex.lock();
    if (mCurThreadInfo) status = mCurThreadInfo->conn_status;
    mMutex.unlock();
    return status;
}

void WifiSta::exit(size_t handle) {
    mMutex.lock();
    if (handle == 0 && mCurThreadInfo) {
        mCurThreadInfo->running = false;
    } else {
        auto it = mHandles.find(handle);
        if (it != mHandles.end()) {
            (reinterpret_cast<ThreadInfo*>(*it))->running = false;
        }
    }
    mMutex.unlock();
}

void WifiSta::down() {
#if defined(HV_FUNCTION_WIFI) && HV_FUNCTION_WIFI
    HV_POPEN("wpa_cli -i " HV_WLAN_NAME " disable_network 0 && wpa_cli -i " HV_WLAN_NAME " remove_network 0");
    HV_POPEN("ifconfig " HV_WLAN_NAME " down");
#endif
}

void WifiSta::up(){
#if defined(HV_FUNCTION_WIFI) && HV_FUNCTION_WIFI
    HV_POPEN("ifconfig " HV_WLAN_NAME " up");
#endif
}
