
#include "hv_wifi_sta.h"

#include <cstdio>
#include <iostream>
#include <regex>
#include <sys/prctl.h>
#include <unistd.h>
#include <unordered_set>

#include <comm_func.h>
#include <hv_series_conf.h>

#include <core/textutils.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "hv_icmp.h"
#include "hv_net.h"
#ifdef __cplusplus
}
#endif

using namespace std;

// 由于文件系统限制。只能指定工作目录运行
#define WIFI_CMD_DIR "/config/wifi/"

#define WPA_WORK_DIR "/etc"

#define WPA_SUPPLICANT_SCRIPT WPA_WORK_DIR"/wpa_supplicant.conf"

#define WPA_SUPPLICANT_TIMEOUT_MS 60;

#define USE_WIFIMANAGER 0
#define USE_WPA_CLI_2_SCAN 1

void parseWifiScan(std::list<WifiSta::WIFI_ITEM_S> &wifiCells, const std::string &scanString) {
    std::istringstream iss(scanString);
    std::string line;
    WifiSta::WIFI_ITEM_S currentCell;
    bool inCell = false;
    bool repNameFlag = false;
    
    while (std::getline(iss, line)) {
        // 检测新的BSS块开始
        if (line.find("BSS") == 0) {
            // 如果已经有正在解析的cell，先保存
            if (inCell && !currentCell.ssid.empty()) {
                repNameFlag = false;
                for (auto item : wifiCells) {
                    if (item.ssid == currentCell.ssid) { 
                        repNameFlag = true;
                        break; 
                    }
                }
                if(!repNameFlag) wifiCells.push_back(currentCell);
            }
            
            // 开始新的cell
            currentCell = WifiSta::WIFI_ITEM_S();
            inCell = true;
            currentCell.isKey = false;
            
            // BSS后面的部分就是MAC地址，格式如：BSS ca:26:a2:08:f1:4c(on wlan0)
            size_t bssPos = line.find("BSS");
            if (bssPos != std::string::npos) {
                // 跳过"BSS"和后面的空格
                size_t macStart = bssPos + 3;
                while (macStart < line.length() && std::isspace(line[macStart])) {
                    macStart++;
                }
                
                // MAC地址长度是17个字符，找到MAC地址的结束位置
                size_t macEnd = macStart;
                while (macEnd < line.length() && 
                       (std::isxdigit(line[macEnd]) || line[macEnd] == ':')) {
                    macEnd++;
                }
                
                if (macEnd - macStart >= 17) { // MAC地址最小长度
                    currentCell.mac = line.substr(macStart, 17);
                }
            }
        }else if (inCell) {
            // 解析信号强度
            if (line.find("signal:") != std::string::npos) {
                size_t start = line.find("signal:") + 7;
                size_t end = line.find("dBm");
                if (start != std::string::npos && end != std::string::npos) {
                    std::string signalStr = line.substr(start, end - start);
                    signalStr.erase(std::remove(signalStr.begin(), signalStr.end(), ' '), signalStr.end());
                    currentCell.quality = std::stoi(signalStr);
                    currentCell.signal = calculation_signal((float)-currentCell.quality/100,-10);
                }
            }
            // 解析SSID
            else if (line.find("SSID:") != std::string::npos) {
                size_t start = line.find("SSID:") + 5;
                while (start < line.length() && line[start] == ' ') start++;
                currentCell.ssid = line.substr(start);
            }
            // 检查加密
            else if (line.find("RSN:") != std::string::npos || line.find("WPA:") != std::string::npos) {
                currentCell.isKey = true;
            }
        }
    }
    
    // 添加最后一个cell
    if (inCell && !currentCell.ssid.empty()) {
        repNameFlag = false;
        for (auto item : wifiCells) {
            if (item.ssid == currentCell.ssid) { 
                repNameFlag = true;
                break; 
            }
        }
        if(!repNameFlag) wifiCells.push_back(currentCell);
    }
}

void parseWifiScan2(std::list<WifiSta::WIFI_ITEM_S> &wifiCells, const std::string &scanString){
    std::istringstream iss(scanString);
    std::string line;
    bool firstLine = true; // 跳过表头

    while (std::getline(iss, line)) {
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        
        WifiSta::WIFI_ITEM_S currentCell;
        currentCell.isKey = false;
        
        // 解析每行的字段
        std::istringstream lineStream(line);
        std::string bssid, frequencyStr, signalStr, flags, ssid;
        
        // 按空格分割字段
        lineStream >> bssid >> frequencyStr >> signalStr;
                
        // 剩余的部分
        std::string remaining;
        std::getline(lineStream, remaining);
        size_t last_bracket = remaining.find_last_of(']');
        if (last_bracket != std::string::npos) {
            // flags是从开始到最后一个']'+1
            std::string flags = remaining.substr(0, last_bracket + 1);
            
            // SSID是剩余部分，去除前后空格
            std::string ssid = remaining.substr(last_bracket + 1);
            ssid.erase(0, ssid.find_first_not_of(" \t"));
            ssid.erase(ssid.find_last_not_of(" \t") + 1);
            
            // 填充currentCell
            currentCell.mac = bssid;
            currentCell.ssid = ssid;
            currentCell.signalLevel = std::stoi(signalStr);
            currentCell.quality = -50;
            currentCell.signal = calculation_signal((float)-currentCell.quality/100, currentCell.signalLevel);

            // 检查是否有密码（通过flags字段）
            if (flags.find("WPA") != std::string::npos || 
                flags.find("WEP") != std::string::npos ||
                flags.find("PSK") != std::string::npos) {
                currentCell.isKey = true;
            }

            // 检查是否重复SSID
            bool repNameFlag = false;
            for (const auto& item : wifiCells) {
                if (item.ssid == currentCell.ssid) {
                    repNameFlag = true;
                    break;
                }
            }
            
            if (!repNameFlag) {
                wifiCells.push_back(currentCell);
            }
        }
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
        fprintf(pFile, "ctrl_interface=%s/wpa_supplicant\n", WPA_WORK_DIR);
        fprintf(pFile, "update_config=1\n");
        fprintf(pFile, "ap_scan=1\n");
        fprintf(pFile, "network={\n");
        fprintf(pFile, "\tssid=\"%s\"\n", ssid.c_str());
        fprintf(pFile, "\tscan_ssid=1\n");
        // 判断密码有无来决定wifi配置文件加密方式
        fprintf(stderr, "key:%d\n", (int)key.length());
        if (key.length() == 0) // NONE
        {
            printf("no passwd!!\n");
            fprintf(pFile, "\tkey_mgmt=NONE\n");
        } else {
            printf("need passwd!! default WPA-PSK\r\n");
            fprintf(pFile, "\tkey_mgmt=WPA-PSK WPA-EAP IEEE8021X NONE\n");
            fprintf(pFile, "\tpairwise=TKIP CCMP\n");
            fprintf(pFile, "\tgroup=CCMP TKIP WEP104 WEP40\n");
            fprintf(pFile, "\tpsk=\"%s\"\n", key.c_str());
        }
        fprintf(pFile, "}");
        fclose(pFile);
        pFile = NULL;
    } else {
        printf("open %s Error!!!\n", WPA_SUPPLICANT_SCRIPT);
        return -1;
    }
    return 0;
}

static int __save_empty_conf() {
    FILE *pFile = NULL;

    pFile = fopen(WPA_SUPPLICANT_SCRIPT, "wt");
    if (pFile == nullptr) {
        perror("open file wpa config error:");
        return -1;
    }

    if (pFile != NULL) {
        fprintf(pFile, "ctrl_interface=%s/wpa_supplicant\n", WPA_WORK_DIR);
        fprintf(pFile, "update_config=1\n");
        fprintf(pFile, "ap_scan=1\n");
        fprintf(pFile, "network={\n");
        fprintf(pFile, "\tscan_ssid=1\n");
        fprintf(pFile, "\tkey_mgmt=NONE\n");
        fprintf(pFile, "}");
        fclose(pFile);
        pFile = NULL;
    } else {
        printf("open %s Error!!!\n", WPA_SUPPLICANT_SCRIPT);
        return -1;
    }
    return 0;
}

/// @brief 重新启动wpa进程
static void __kill_wpa() {
    HV_POPEN("killall -9 udhcpc");
    HV_POPEN("ifconfig wlan0 down");
    HV_POPEN("ifconfig wlan0 up");
    HV_POPEN("killall -9 wpa_supplicant");
}

/*****************************************************/
WifiSta::WifiSta(/* args */) {
    mCurThreadInfo = 0;
    mWifiConnStatus= E_DISCONNECT;
#if HV_FUNCTION_WIFI == 1
    // // const char *command = WIFI_CMD_DIR "ssw01bInit.sh";
    // const char *command = "insmod /lib/modules/5.4.61/8723ds.ko";
    // FILE       *fp      = popen(command, "r");
    // if (fp == nullptr) { std::cout << "exe cmd error:" << command << std::endl; }
    // pclose(fp);
    // const char *command1 = "ifconfig wlan0 down";
    // fp                   = popen(command1, "r");
    // pclose(fp);
    // const char *command2 = "ifconfig wlan0 up";
    // fp                   = popen(command2, "r");
    // pclose(fp);
#if USE_WIFIMANAGER == 1
    HV_POPEN("insmod /lib/modules/5.4.61/8723ds.ko");
    HV_POPEN("wifi_deamon -d 6");
    enableWifi();
#else
    HV_POPEN("insmod /lib/modules/5.4.61/8723ds.ko");
    HV_POPEN("ifconfig wlan0 down");
    HV_POPEN("ifconfig wlan0 up");
    __save_empty_conf();
    // 启动脚本链接wifi
    system("wpa_supplicant -B -Dnl80211 -i" HV_WLAN_NAME " -c" WPA_SUPPLICANT_SCRIPT);
#endif //USE_WIFIMANAGER
#endif
}

WifiSta::~WifiSta() {
    if (mCurThreadInfo) { mCurThreadInfo->running = false; }
}

/// @brief
/// @param wifi_lsit
/// @return
int32_t WifiSta::scan(std::list<WIFI_ITEM_S> &wifilist) {
#if HV_FUNCTION_WIFI == 1
// #define MAX_BUFFER_SIZE 1024 * 128 * 10
//     char        buffer[MAX_BUFFER_SIZE] = {0};
    // const char *command                 = WIFI_CMD_DIR "iwlist " HV_WLAN_NAME " scan";
#if USE_WIFIMANAGER
    // 执行扫描命令
    std::string result = executeCommandWithOutput("wifi -s");
    // 解析扫描结果
    std::istringstream iss(result);
    std::string line;
    
    std::list<WIFI_ITEM_S> tmp;
    wifilist.swap(tmp);
    
    // 正则表达式匹配bss格式
    std::regex bssRegex(R"(bss\[(\d+)\]: bssid=([0-9a-fA-F:]+)\s+ssid=([^\s]+)\s+channel=(\d+)\(freq=(\d+)\)\s+rssi=(-?\d+)\s+sec=([^\s]+))");
    // 定义不需要密码的安全类型
    static const std::unordered_set<std::string> openNetworks = {
        "NONE", "OPEN", "NONE_OPEN", "OPEN_NONE"
    };
    
    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, bssRegex)) {
            WifiSta::WIFI_ITEM_S currentCell;
            int index = std::stoi(match[1]);
            int channel = std::stoi(match[4]);
            int frequency = std::stoi(match[5]);
            currentCell.mac = match[2];
            currentCell.ssid = match[3];
            currentCell.signalLevel = std::stoi(match[6]);
            currentCell.quality = -50;
            currentCell.signal = calculation_signal((float)-currentCell.quality/100,currentCell.signalLevel);
            // 如果是不需要密码的类型
            if (openNetworks.find(match[7]) != openNetworks.end()) {
                currentCell.isKey = false;
            }else {
                currentCell.isKey = true;
            }
            
            wifilist.push_back(currentCell);
        }
    }
#else //USE_WIFIMANAGER

#if USE_WPA_CLI_2_SCAN
    std::string command = "pidof wpa_supplicant  > /dev/null 2>&1";
    if (std::system(command.c_str()) != 0){
        __save_empty_conf();
        // 启动脚本链接wifi
        system("wpa_supplicant -B -Dnl80211 -i" HV_WLAN_NAME " -c" WPA_SUPPLICANT_SCRIPT);
    }
    system("wpa_cli -i" HV_WLAN_NAME " -p /etc/wpa_supplicant scan");
    sleep(3);
    command = "wpa_cli -i" HV_WLAN_NAME " -p /etc/wpa_supplicant scan_results";
    FILE       *fp                      = popen(command.c_str(), "r");
    if (fp == nullptr) {
        std::cout << "exe cmd error:" << command << std::endl << std::strerror(errno);
        return -1;
    }
    // fread(buffer, 1, MAX_BUFFER_SIZE - 1, fp);
    std::string result;
    char line[256];
    while (fgets(line, sizeof(line), fp) != nullptr) {
        result += line;
    }
    LOGE("fread ~~~~ result.size = %d",result.size());
    pclose(fp);
    std::list<WIFI_ITEM_S> tmp;
    wifilist.swap(tmp);
    parseWifiScan2(wifilist, result);
#else //USE_WPA_CLI_2_SCAN

    const char *command                 = "iw dev wlan0 scan";
    FILE       *fp                      = popen(command, "r");
    if (fp == nullptr) {
        std::cout << "exe cmd error:" << command << std::endl << std::strerror(errno);
        return -1;
    }

    // fread(buffer, 1, MAX_BUFFER_SIZE - 1, fp);
    std::string result;
    char line[256];
    while (fgets(line, sizeof(line), fp) != nullptr) {
        result += line;
    }
    LOGE("fread ~~~~ result.size = %d",result.size());
    pclose(fp);
    std::list<WIFI_ITEM_S> tmp;
    wifilist.swap(tmp);
    parseWifiScan(wifilist, result);
#endif//USE_WPA_CLI_2_SCAN

#endif// USE_WIFIMANAGER
    // for (const auto &cell : wifilist) {
    //     std::cout << "Address: " << cell.mac << std::endl;
    //     std::cout << "ESSID: " << cell.ssid << std::endl;
    //     std::cout << "isKey: " << cell.isKey << std::endl;
    //     std::cout << "quality: " << cell.quality << std::endl;
    //     std::cout << "signal: " << cell.signal << std::endl;
    // }
#else//HV_FUNCTION_WIFI == 1
    std::list<WIFI_ITEM_S> tmp;
    wifilist.swap(tmp);
    for (uint8_t i = 0; i < 10; i++) {
        WIFI_ITEM_S item;
        item.ssid = "test wifi " + std::to_string(i);
        item.mac  = "88:88:88:88:" + std::to_string(i);
        item.rssi = -86 + __rand_value(1, 10);
        item.isKey = true;
        item.signal = (i%4)+1;
        wifilist.push_front(item);
    }
#endif//HV_FUNCTION_WIFI == 1
    // 信号排序
    wifilist.sort([](const WIFI_ITEM_S &a, const WIFI_ITEM_S &b) -> bool { 
        if(a.signal == b.signal)            return a.signalLevel > b.signalLevel;
        else                                return a.signal > b.signal;
    });
    // for (const auto &cell : wifilist) {
    //     std::string ssidText = cell.ssid;
    //     LOGI("ssid = %s  signalLevel = %d  quality = %d",fillLength(ssidText,30,' ').c_str(),cell.signalLevel,cell.quality);
    // }
    return 0;
}

/// @brief 链接
/// @param name
/// @param key
/// @return
int32_t WifiSta::connect(const std::string &name, const std::string &key) {
    mMutex.lock();
    if (mCurThreadInfo) { mCurThreadInfo->running = false; }
    mMutex.unlock();

#if HV_FUNCTION_WIFI == 1
    // 保存配置文件。连接wifi
    __save_conf(name, key);
    mWifiConnStatus    = E_STA_CONENCTING;
#else
    mWifiConnStatus = E_CONNECT_SUCCESS;
#endif
    mCurThreadInfo     = new ThreadInfo(name, key);
#if USE_WIFIMANAGER
    connect_wifimanager_run(mCurThreadInfo);
#else
    conenct_run(mCurThreadInfo);
#endif
    return 0;
}

/**
 * 处理 WPA 的控制状态。
 *
 */
WifiSta::CONENCT_STATUS_E WifiSta::handle_result(CONENCT_STATUS_E old, struct wifi_status_result_t &result) {
    CONENCT_STATUS_E state = old;
    printf("wifi sta state:%s\r\n", result.wpa_state);
    if (strcmp(result.wpa_state, "COMPLETED") == 0) {
        state = E_STA_CONENCTINGED;
    } else if (strcmp(result.wpa_state, "SCANNING") == 0) {
        // 校验超时。
        state = E_STA_CONENCTING;
    } else {
    }
    return state;
}

//
void WifiSta::conenct_run(ThreadInfo *lpInfo) {
#if HV_FUNCTION_WIFI == 1
    int32_t connectTimeOut = WPA_SUPPLICANT_TIMEOUT_MS;
    char    szGateway[16]  = {0};
    prctl(PR_SET_NAME, __func__);
    uint8_t pingCnt = 0;

    wpa_ctrl_run_set_path("/etc");
GOTO_CONENNCTION_REFUSED:
    if (lpInfo->softResetCnt && lpInfo->softResetCnt <= 5) {
        // HV_POPEN(WIFI_CMD_DIR "ssw01bInit.sh");
    } else if (lpInfo->softResetCnt > 5) {
        while (lpInfo->running) {
            printf("%s abnormal. May require a reboot\r\n", HV_WLAN_NAME);
            usleep(200);
        }
        mMutex.lock();
        if (lpInfo == mCurThreadInfo) { mCurThreadInfo = 0; }
        delete lpInfo;
        mMutex.unlock();
        return;
    }

    lpInfo->softResetCnt++;

    //
    __kill_wpa();

    // 启动脚本链接wifi
    system("wpa_supplicant -B -Dnl80211 -i" HV_WLAN_NAME " -c" WPA_SUPPLICANT_SCRIPT);
    lpInfo->conn_status = E_STA_CONENCTING;
    struct timespec stTime;
    clock_gettime(CLOCK_MONOTONIC, &stTime);
    uint32_t old_timecount = stTime.tv_sec;
    printf("wpa_ctrl:conenct_run start\r\n");
    pingCnt = 0;
    while (lpInfo->running) {
        clock_gettime(CLOCK_MONOTONIC, &stTime);

        switch (lpInfo->conn_status) {
        case E_STA_CONENCTING:
            // 查询状态
            struct wifi_status_result_t result;
            if (wpa_ctrl_sta_status(&result) == 0) {
                lpInfo->conn_status = handle_result(lpInfo->conn_status, result);
                old_timecount       = stTime.tv_sec;
            }

            if ((stTime.tv_sec > old_timecount && stTime.tv_sec - old_timecount > 60)) {
                printf("connection timed out\r\n");
                int rssi;
                if (wpa_ctrl_sta_get_near_ap_list(lpInfo->ssid.c_str(), &rssi) == 0) {
                    lpInfo->conn_status = E_STA_CONENCTING;
                } else {
                    printf("ap not find\r\n");
                }
                goto GOTO_CONENNCTION_REFUSED;
            }
            /* code */
            break;
        // 连接完成 分配或设置IP
        case E_STA_CONENCTINGED:
            system("udhcpc -i " HV_WLAN_NAME " -s /usr/share/udhcpc/default.script"); 
            lpInfo->conn_status = E_CONENCT_STA_DHCP;
            break;
        // 检查IP 地址 和 MAC 不为空
        case E_CONENCT_STA_DHCP:

            char szIP[16];
            char szMac[32];
            HV_NET_GetIpAddr(HV_WLAN_NAME, szIP);
            HV_NET_GetMacAddr(HV_WLAN_NAME, NULL, szMac);

            if (szIP[0] == '\0') {
                // 软复位
                printf("no correct ip!!!!,softreset\r\n");
                goto GOTO_CONENNCTION_REFUSED;
            }

            if (szMac[0] == '\0') {
                // 软复位
                printf("no correct mac!!!!,softreset\r\n");
                goto GOTO_CONENNCTION_REFUSED;
            }

            if (szGateway[0] == 0) { HV_NET_GetGateway(HV_WLAN_NAME, (char *)szGateway); }
            lpInfo->conn_status = E_CONENCT_STA_GUARD;
            break;
        case E_CONENCT_STA_GUARD:

            // 还没获取到网关
            if (szGateway[0] == 0) {
                HV_NET_GetGateway(HV_WLAN_NAME, (char *)szGateway);
                if (szGateway[0] == 0) { goto GOTO_CONENNCTION_REFUSED; }
            }

            if (stTime.tv_sec - old_timecount > 5) {
                old_timecount = stTime.tv_sec;

                if (ping((char *)szGateway, 1) > 0) {
                    pingCnt = 0;
                } else {
                    pingCnt++;
                    printf("NetWork ping failed cnt:%d %s\r\n", pingCnt, szGateway);
                }
            }

            if (pingCnt > 10) {
                // 网卡异常
                if (HV_NET_CheckNicStatusUp(HV_WLAN_NAME) != 0) {
                    printf("Network card abnormality\r\n");
                    goto GOTO_CONENNCTION_REFUSED;
                }

                // 网络连接异常
                else if (HV_NET_CheckNicStatus(HV_WLAN_NAME) != 0) {
                    // 重新连接wifi
                    printf("Network link abnormality\r\n");
                    goto GOTO_CONENNCTION_REFUSED;
                }
            }
            mWifiConnStatus = E_CONNECT_SUCCESS;
            break;
        default: break;
        }
        usleep(500*1000);
        LOGE("mWifiConnStatus = %d lpInfo->conn_status = %d",mWifiConnStatus,lpInfo->conn_status);
        if(mWifiConnStatus == E_CONNECT_SUCCESS) break;
    }
#endif

    mMutex.lock();    
    if (lpInfo == mCurThreadInfo) { mCurThreadInfo = 0; }
    delete lpInfo;
    mMutex.unlock();
}

void WifiSta::connect_wifimanager_run(ThreadInfo *lpInfo){

    std::string command = "wifi -c \"" + lpInfo->ssid + "\"";
    if (!lpInfo->key.empty()) {
        command += " \"" + lpInfo->key + "\"";
    }
    
    // 执行连接命令并获取输出
    std::string result = executeCommandWithOutput(command);
    
    // 分析输出判断是否连接成功
    if (result.find("Wi-Fi connect failed") != std::string::npos ||
        result.find("WRONG_KEY") != std::string::npos ||
        result.find("4-Way Handshake failed") != std::string::npos) {
        return;
    }
    
    // 进一步检查连接状态
    if(checkConnectionStatus(lpInfo->ssid)){
        mWifiConnStatus = E_CONNECT_SUCCESS;
    }
}
WifiSta::CONENCT_STATUS_E WifiSta::get_status() {
    WifiSta::CONENCT_STATUS_E status = mWifiConnStatus;
    mMutex.lock();    
    if (mCurThreadInfo) { status = mCurThreadInfo->conn_status; }    
    mMutex.unlock();
    return status;
}

void WifiSta::disconnect() {
#if USE_WIFIMANAGER
     // 断开当前连接
    executeCommand("wifi -d");
#else
    mMutex.lock();
    if (mCurThreadInfo) { mCurThreadInfo->running = false; }
    system("wpa_cli -i" HV_WLAN_NAME " -p /etc/wpa_supplicant disconnect");
    mMutex.unlock();
    mWifiConnStatus = E_DISCONNECT;
#endif
}

// void WifiSta::resetWifi(){
    
//     // 启动脚本链接wifi
//     // disconnect();
//     __kill_wpa();
//     system(WIFI_CMD_DIR "wpa_supplicant -B -Dnl80211 -i" HV_WLAN_NAME " -c" WPA_SUPPLICANT_SCRIPT);
// }

void WifiSta::disEnableWifi(){
#if USE_WIFIMANAGER
     // 断开Sta
    executeCommand("wifi -f");
#else
    HV_POPEN("ifconfig wlan0 down");
#endif
}
void WifiSta::enableWifi(){
    // HV_POPEN(WIFI_CMD_DIR "ssw01bInit.sh"); 
#if USE_WIFIMANAGER
    // 打开Sta
    executeCommand("wifi -o sta");
#else
    HV_POPEN("ifconfig wlan0 up");
#endif
}


int WifiSta::executeCommand(const std::string& command) {
    return system((command + " > /dev/null 2>&1").c_str());
}

std::string WifiSta::executeCommandWithOutput(const std::string& command) {
    std::string result;
    char buffer[128];
    
    FILE* pipe = popen((command).c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

// 检查当前连接状态
bool WifiSta::checkConnectionStatus(const std::string& expectedSSID) {
    // 使用系统命令检查连接状态
    std::string result = executeCommandWithOutput("wifi -l");
    
    if (!expectedSSID.empty()) {
        // 检查是否连接到指定SSID
        return result.find(expectedSSID) != std::string::npos;
    }
    
    // 简单检查是否有连接
    return result.find("connected") != std::string::npos || 
            !result.empty();
}
