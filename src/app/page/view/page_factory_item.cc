/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-03 17:05:27
 * @LastEditTime: 2026-07-05 02:24:24
 * @FilePath: /kk_frame/src/app/page/view/page_factory_item.cc
 * @Description: 产测页面子项
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory_item.h"

#include "wifi_mgr.h"
#include "auth_mgr.h"
#include "config_mgr.h"

#include "touch_test_view.h"

#include "string_utils.h"
#include "arg_utils.h"
#include "network_utils.h"
#include "brand_utils.h"

#include "config_info.h"
#include "global_data.h"
#include "time_utils.h"

#include "app_version.h"
#include "series_info.h"
#include <core/build.h>
#include <core/systemclock.h>

#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/************************** 系统信息 **************************/

namespace {

    /// @brief 读取文本文件内容（适用于 /proc、/sys 等虚拟文件）
    bool readTextFile(const std::string& path, std::string& content) {
        FILE* fp = fopen(path.c_str(), "r");
        if (!fp) return false;

        content.clear();
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp)) content.append(buffer);
        fclose(fp);
        return true;
    }

    /// @brief 读取 "Key: Value" 格式文本中的数值
    /// @param content 文本内容
    /// @param key 字段名
    /// @return 字段数值，未找到返回 0
    uint64_t readFieldValue(const std::string& content, const char* key) {
        const std::string tag = std::string(key) + ":";
        const size_t pos = content.find(tag);
        if (pos == std::string::npos) return 0;
        return strtoull(content.c_str() + pos + tag.size(), nullptr, 10);
    }

    /// @brief 容量格式化
    /// @param bytes 字节数
    /// @return 例如 512.0 MB
    std::string formatSize(uint64_t bytes) {
        static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
        double value = static_cast<double>(bytes);
        int unit = 0;
        while (value >= 1024.0 && unit < 4) {
            value /= 1024.0;
            ++unit;
        }
        return StringUtils::format("%.1f %s", value, units[unit]);
    }

    /// @brief 时长格式化
    /// @param seconds 秒数
    /// @return 例如 3天 05:21:08 / 05:21:08
    std::string formatDuration(uint64_t seconds) {
        const uint64_t days = seconds / TimeUtils::DAY_SECONDS;
        seconds %= TimeUtils::DAY_SECONDS;
        const uint64_t hours = seconds / TimeUtils::HOUR_SECONDS;
        seconds %= TimeUtils::HOUR_SECONDS;
        const uint64_t minutes = seconds / TimeUtils::MINUTE_SECONDS;
        const uint64_t secs = seconds % TimeUtils::MINUTE_SECONDS;

        if (days > 0) {
            return StringUtils::format("%llu天 %02llu:%02llu:%02llu",
                static_cast<unsigned long long>(days),
                static_cast<unsigned long long>(hours),
                static_cast<unsigned long long>(minutes),
                static_cast<unsigned long long>(secs));
        }
        return StringUtils::format("%02llu:%02llu:%02llu",
            static_cast<unsigned long long>(hours),
            static_cast<unsigned long long>(minutes),
            static_cast<unsigned long long>(secs));
    }

    /// @brief 分区容量信息
    struct DiskUsage {
        bool     valid{ false };      // 是否获取成功
        uint64_t total{ 0 };          // 总容量（字节）
        uint64_t available{ 0 };      // 可用容量（字节）
    };

    /// @brief 获取指定路径所在分区的容量信息
    DiskUsage getDiskUsage(const std::string& path) {
        DiskUsage usage;
        struct statvfs stat = {};
        if (statvfs(path.c_str(), &stat) != 0) return usage;

        usage.valid = true;
        usage.total = static_cast<uint64_t>(stat.f_blocks) * stat.f_frsize;
        usage.available = static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
        return usage;
    }

    /// @brief 获取 CPU 型号
    std::string getCpuModel() {
        std::string content;
        if (!readTextFile("/proc/cpuinfo", content)) return "-";

        std::string fallback;  // 备用值（Processor 等信息）
        size_t pos = 0;
        while (pos < content.size()) {
            size_t end = content.find('\n', pos);
            if (end == std::string::npos) end = content.size();
            const std::string line = content.substr(pos, end - pos);
            pos = end + 1;

            const size_t sep = line.find(':');
            if (sep == std::string::npos) continue;

            const std::string value = StringUtils::trimRight(line.substr(sep + 1));
            if (value.empty()) continue;

            if (line.compare(0, strlen("Hardware"), "Hardware") == 0
                || line.compare(0, strlen("model name"), "model name") == 0) {
                return value;
            }
            if (fallback.empty()
                && (line.compare(0, strlen("Processor"), "Processor") == 0
                    || line.compare(0, strlen("cpu model"), "cpu model") == 0)) {
                fallback = value;
            }
        }
        return fallback.empty() ? "-" : fallback;
    }

    /// @brief 获取 CPU 温度
    /// @param celsius 温度（摄氏度）
    /// @return true 获取成功
    bool getCpuTemperature(double& celsius) {
        for (int i = 0; i < 8; ++i) {
            const std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
            FILE* fp = fopen(path.c_str(), "r");
            if (!fp) continue;

            long value = 0;
            const bool ok = (fscanf(fp, "%ld", &value) == 1);
            fclose(fp);
            if (!ok || value <= 0) continue;

            // 部分驱动以毫摄氏度上报，部分直接上报摄氏度
            celsius = (value > 1000) ? (value / 1000.0) : static_cast<double>(value);
            return true;
        }
        return false;
    }

} // namespace

FactorySystem::FactorySystem(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);

    mSystemInfo = PBase::get<TextView>(mRootView, AppRid::system_info);
    PBase::click(mRootView, AppRid::refresh, [this](View&) {setInfo();});

    mTicker.setTick(1000);
    mTicker.setCallBack(std::bind(&FactorySystem::onTick, this, std::placeholders::_1));
    setInfo();
}

FactorySystem::~FactorySystem() {
    mTicker.stopTick();
}

void FactorySystem::onShow() {
    setInfo();
    mTicker.startTick();
}

void FactorySystem::onHide() {
    mTicker.stopTick();
}

void FactorySystem::onTick(int64_t) {
    setInfo();
}

void FactorySystem::setInfo() {
    std::string info;

    // 时间信息
    const time_t nowSec = TimeUtils::getTimeSec();
    uint64_t uptimeSec = 0;
    std::string uptimeText;
    if (readTextFile("/proc/uptime", uptimeText)) {
        uptimeSec = strtoull(uptimeText.c_str(), nullptr, 10);
    }

    info += "当前时间: " + TimeUtils::getTimeFmtStr("%Y-%m-%d %H:%M:%S");
    const std::string week = TimeUtils::getDayOnWeek();
    if (!week.empty()) info += " " + week;
    info += "\n";

    if (nowSec > 0 && uptimeSec > 0 && static_cast<uint64_t>(nowSec) > uptimeSec) {
        info += "开机时间: " + TimeUtils::getTimeFmtStr(
            static_cast<time_t>(nowSec - uptimeSec), "%Y-%m-%d %H:%M:%S") + "\n";
    }
    info += "系统运行: " + formatDuration(uptimeSec) + "\n";

    const uint64_t nowMs = static_cast<uint64_t>(cdroid::SystemClock::uptimeMillis());
    const uint64_t appRunSec = (nowMs > g_data->mAppStart) ? (nowMs - g_data->mAppStart) / 1000 : 0;
    info += "程序运行: " + formatDuration(appRunSec) + "\n";

    info += "\n";

    // 硬件信息
    struct utsname uts = {};
    if (uname(&uts) == 0) {
        info += StringUtils::format("主机名称: %s\n", uts.nodename);
        info += StringUtils::format("内核版本: %s %s %s\n", uts.sysname, uts.release, uts.machine);
    }
    info += "CPU 型号: " + getCpuModel() + "\n";

    const long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    info += StringUtils::format("CPU 核心: %ld\n", cpuCount > 0 ? cpuCount : 0);

    std::string loadText;
    double load1 = 0, load5 = 0, load15 = 0;
    if (readTextFile("/proc/loadavg", loadText)
        && sscanf(loadText.c_str(), "%lf %lf %lf", &load1, &load5, &load15) == 3) {
        info += StringUtils::format("系统负载: %.2f / %.2f / %.2f\n", load1, load5, load15);
    }

    double temperature = 0;
    if (getCpuTemperature(temperature)) {
        info += StringUtils::format("CPU 温度: %.1f ℃\n", temperature);
    }

    info += "\n";

    // 内存信息
    std::string memText;
    if (readTextFile("/proc/meminfo", memText)) {
        const uint64_t totalKb = readFieldValue(memText, "MemTotal");
        uint64_t availableKb = readFieldValue(memText, "MemAvailable");
        if (availableKb == 0) {
            // 老内核无 MemAvailable，退化为 Free + Buffers + Cached
            availableKb = readFieldValue(memText, "MemFree")
                + readFieldValue(memText, "Buffers")
                + readFieldValue(memText, "Cached");
        }
        if (totalKb > 0) {
            info += "内存总计: " + formatSize(totalKb * 1024) + "\n";
            info += "内存可用: " + formatSize(availableKb * 1024) + "\n";
            info += StringUtils::format("内存占用: %.1f%%\n",
                (totalKb - availableKb) * 100.0 / totalKb);
        }

        const uint64_t swapTotalKb = readFieldValue(memText, "SwapTotal");
        const uint64_t swapFreeKb = readFieldValue(memText, "SwapFree");
        if (swapTotalKb > 0) {
            info += "交换分区: " + formatSize((swapTotalKb - swapFreeKb) * 1024)
                + " / " + formatSize(swapTotalKb * 1024) + "\n";
        }
    }

    // 进程信息
    std::string selfText;
    if (readTextFile("/proc/self/status", selfText)) {
        const uint64_t rssKb = readFieldValue(selfText, "VmRSS");
        if (rssKb > 0) info += "程序内存: " + formatSize(rssKb * 1024) + "\n";

        const uint64_t threads = readFieldValue(selfText, "Threads");
        if (threads > 0) {
            info += StringUtils::format("线程数量: %llu\n",
                static_cast<unsigned long long>(threads));
        }
    }
    info += StringUtils::format("进程 PID: %d\n", getpid());

    info += "\n";

    // 储存信息
    auto appendDisk = [&info](const char* name, const DiskUsage& disk) {
        if (!disk.valid || disk.total == 0) {
            info += StringUtils::format("%s: -\n", name);
            return;
        }
        info += StringUtils::format("%s: %s 可用 / %s (剩余 %.1f%%)\n",
            name,
            formatSize(disk.available).c_str(),
            formatSize(disk.total).c_str(),
            disk.available * 100.0 / disk.total);
    };
    appendDisk("根分区", getDiskUsage("/"));
    appendDisk("数据分区", getDiskUsage(APP_RW_DIR));

    mSystemInfo->setText(info.c_str());
}

/************************** 项目信息 **************************/

FactoryInfo::FactoryInfo(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setInfo();
}

void FactoryInfo::setInfo() {
    TextView* tv = PBase::get<TextView>(mRootView, AppRid::info);
    std::string info;

    // 软件信息
    info += "软件名称: " APP_NAME_STR "\n";
    info += "软件版本: V" APP_VERSION_D "\n";
    info += StringUtils::format("框架版本: Cdroid_V%s_%s\n", cdroid::Build::VERSION::Release, cdroid::Build::VERSION::CommitID);
    info += "代码哈希: " GIT_VERSION "\n";
    info += StringUtils::format("打包人员: %s\n", getlogin());
    info += "打包时间: " BUILD_DATE "\n";
    info += "打包详情: " APP_VER_INFO "\n";
    info += "运行命令: " + ArgUtils::getRawString() + "\n";
    info += "版权署名: " + BrandUtils::copyrightLine() + "\n";

    info += "\n";

    // 硬件信息
    info += "核心: " CPU_BRAND " " CPU_NAME "\n";
    info += "内存: " MEMORY_NAME " " MEMORY_SIZE "\n";
    info += "闪存: " FLASH_NAME " " FLASH_SIZE "\n";
    info += "屏幕: " SCREEN_SIZE "\n";

    tv->setText(info.c_str());
}

/************************** 网络信息 **************************/

FactoryNetwork::FactoryNetwork(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setInterface();
    PBase::click(mRootView, AppRid::refresh, [this](View&) {setInterface();});
}

void FactoryNetwork::setInterface() {
    TextView* interFace = PBase::get<TextView>(mRootView, AppRid::inter_face);
    std::string info;

    // 网络信息
    std::vector<NetworkUtils::InterfaceInfo> netInfo = NetworkUtils::getInterfaces();
    if (netInfo.empty()) {
        info = "未检测到网络接口\n";
    }

    for (const auto& item : netInfo) {
        info += StringUtils::format("[%s]  %s / %s\n",
            item.name.c_str(),
            item.isUp ? "UP" : "DOWN",
            item.isRunning ? "RUNNING" : "STOPPED");
        info += "  当前 MAC: "
            + (item.macAddress.empty() ? std::string("-") : item.macAddress) + "\n";
        info += "  永久 MAC: "
            + (item.permanentMacAddress.empty()
                ? std::string("-") : item.permanentMacAddress) + "\n";
        info += StringUtils::format("  MTU: %u\n", item.mtu);

        for (size_t i = 0; i < item.ipv4Addresses.size(); ++i) {
            std::string suffix;
            if (i < item.ipv4Netmasks.size()) {
                uint8_t prefix = 0;
                if (NetworkUtils::netmaskToPrefixLength(item.ipv4Netmasks[i], prefix)) {
                    suffix = "/" + std::to_string(prefix);
                } else if (!item.ipv4Netmasks[i].empty()) {
                    suffix = "  掩码 " + item.ipv4Netmasks[i];
                }
            }
            info += "  IPv4: " + item.ipv4Addresses[i] + suffix + "\n";
        }

        for (size_t i = 0; i < item.ipv6Addresses.size(); ++i) {
            std::string suffix;
            if (i < item.ipv6Netmasks.size()) {
                uint8_t prefix = 0;
                if (NetworkUtils::netmaskToPrefixLength(item.ipv6Netmasks[i], prefix)) {
                    suffix = "/" + std::to_string(prefix);
                }
            }
            info += "  IPv6: " + item.ipv6Addresses[i] + suffix + "\n";
        }

        if (item.ipv4Addresses.empty() && item.ipv6Addresses.empty()) {
            info += "  IP: -\n";
        }
        info += "\n";
    }

    std::string gateway;
    std::string gatewayInterface;
    if (NetworkUtils::getDefaultGateway(gateway, &gatewayInterface)) {
        info += "默认网关: " + gateway + " (" + gatewayInterface + ")\n";
    } else {
        info += "默认网关: -\n";
    }

    const std::vector<std::string> dnsServers = NetworkUtils::getDnsServers();
    info += "DNS: ";
    if (dnsServers.empty()) {
        info += "-";
    } else {
        for (size_t i = 0; i < dnsServers.size(); ++i) {
            if (i != 0) info += ", ";
            info += dnsServers[i];
        }
    }
    info += "\n";

    interFace->setText(info.c_str());
}

/************************** 授权信息 **************************/

FactoryAuth::FactoryAuth(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);

    mAuthInfo = PBase::get<TextView>(mRootView, AppRid::auth_info);
    mAuthRequest = PBase::get<TextView>(mRootView, AppRid::auth_request);
    mAuthRequestInfo = PBase::get<TextView>(mRootView, AppRid::auth_request_info);

    PBase::click(mAuthRequest, [](View&) {
        g_auth->request();
    });

    g_msg->add(MSG_AUTH_CODE, this);
}

FactoryAuth::~FactoryAuth() {
    g_msg->removeAll(this);
}

void FactoryAuth::onShow() {
    onMessage(MSG_AUTH_CODE, 0, nullptr);
}

void FactoryAuth::onMessage(int msgType, int msgValue, void* msgPtr) {
    if (msgType != MSG_AUTH_CODE) return;

    mAuthRequestInfo->setText(
        g_auth->mac() + "\n" + g_auth->authURL()
    );

    if (g_config->isAuthSuccess()) {
        mAuthInfo->setText(
            StringUtils::format(
                "授权码信息\nPIP: %s | DID: %s",
                g_config->getProductId().c_str(),
                g_config->getDeviceId().c_str()
            )
        );
        mAuthRequest->setText("再次请求");
        mAuthRequest->setActivated(true);
    } else {
        mAuthInfo->setText("当前不存在授权码");
        mAuthRequest->setText("申请授权码");
        mAuthRequest->setActivated(false);
    }
}

/************************** 功能开关 **************************/

FactorySwitch::FactorySwitch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setSwitch();
}

void FactorySwitch::setSwitch() { }

FactoryTouch::FactoryTouch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setTouch();
}

/************************** 触摸测试 **************************/

void FactoryTouch::onShow() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->resetTest();
}

void FactoryTouch::setTouch() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->setOnAllBlocksActivated([this]() {
        exit();
    });
}

/************************** 显示测试 **************************/

FactoryColor::FactoryColor(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setColor();
}

void FactoryColor::onShow() {
    cdroid::Drawable* bg = mRootView->getBackground();
    bg->setLevel(1);
}

void FactoryColor::setColor() {
    PBase::click(mRootView, [this](View& v) {
        cdroid::Drawable* bg = v.getBackground();
        int level = bg->getLevel();
        if (level < 7) {
            bg->setLevel(++level);
        } else {
            exit();
        }
    });
}

/************************** WIFI检测 **************************/

FactoryWifi::FactoryWifi(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);

    mWifiList = PBase::get<TextView>(mRootView, AppRid::wifi_list);
    mWifiConnInfo = PBase::get<TextView>(mRootView, AppRid::wifi_conn_info);
    mWifiRefresh = PBase::get<TextView>(mRootView, AppRid::refresh);
    mWifiSwitch = PBase::get<TextView>(mRootView, AppRid::wifi_switch);

    PBase::click(mWifiSwitch, [this](View&v) {
        g_wifi->setSwitch(!v.isActivated());
        updateWifiInfo();
    });
    PBase::click(mWifiRefresh, [this](View&) {g_wifi->scan();});
    PBase::click(mWifiConnInfo, [this](View&) {
#ifdef PRODUCT_X64
        g_wifi->connect("IMKK", "Abcd+1234");
#endif
    });

    g_wifi->addListener(this);
}

FactoryWifi::~FactoryWifi() {
    g_wifi->removeListener(this);
}

void FactoryWifi::onShow() {
    updateWifiInfo();
}

void FactoryWifi::onStateChanged() {
    updateWifiInfo();
}

void FactoryWifi::onScanResult() {
    updateWifiInfo();
}

void FactoryWifi::updateWifiInfo() {
    const bool open = g_wifi->getSwitch();
    mWifiSwitch->setActivated(open);
    mWifiSwitch->setText(open ? "WIFI: ON" : "WIFI: OFF");
    mWifiRefresh->setVisibility(open ? View::VISIBLE : View::GONE);
    mWifiConnInfo->setVisibility(open ? View::VISIBLE : View::GONE);
    mWifiList->setVisibility(open ? View::VISIBLE : View::GONE);

    if (!open) {
        mWifiConnInfo->setText("未连接");
        mWifiConnInfo->setActivated(false);
        mWifiList->setText("");
        return;
    }

    const WifiHal::State state = g_wifi->getState();
    const char* stateText = "空闲";
    switch (state) {
    case WifiHal::State::Off:          stateText = "已关闭"; break;
    case WifiHal::State::Idle:         stateText = "空闲"; break;
    case WifiHal::State::Scanning:     stateText = "扫描中"; break;
    case WifiHal::State::Connecting:   stateText = "连接中"; break;
    case WifiHal::State::Connected:    stateText = "获取 IP 中"; break;
    case WifiHal::State::IpReady:      stateText = "已连接"; break;
    case WifiHal::State::Disconnected: stateText = "连接已断开"; break;
    case WifiHal::State::AuthFailed:   stateText = "认证失败"; break;
    case WifiHal::State::ApNotFound:   stateText = "未找到热点"; break;
    }

    std::vector<WifiHal::ApInfo> aps;
    g_wifi->getAps(aps);

    std::string connection = stateText;
#if 0
    if (state == WifiHal::State::IpReady) {
        for (const auto& ap : aps) {
            if (ap.connected) {
                connection += ": " + ap.ssid;
                break;
            }
        }
    }
#endif
    mWifiConnInfo->setText(connection.c_str());
    mWifiConnInfo->setActivated(state == WifiHal::State::IpReady);

    std::string list;
    if (state == WifiHal::State::Scanning && aps.empty()) {
        list = "正在扫描热点...";
    } else if (aps.empty()) {
        list = "未扫描到热点";
    } else {
        constexpr size_t STATE_WIDTH = 12;
        constexpr size_t SECURITY_WIDTH = 8;
        constexpr size_t SIGNAL_WIDTH = 15;
        constexpr size_t FREQUENCY_WIDTH = 14;
        auto column = [](const std::string& text, size_t width) {
            std::string value = StringUtils::substringByChars(text.c_str(), width, 2);
            const size_t valueWidth = StringUtils::characterCount(value.c_str(), 2);
            value.append(width - valueWidth, ' ');
            return value;
        };

        list = column("状态", STATE_WIDTH)
            + column("安全", SECURITY_WIDTH)
            + column("信号", SIGNAL_WIDTH)
            + column("频率", FREQUENCY_WIDTH)
            + "SSID\n";
        for (const auto& ap : aps) {
            const std::string ssid = ap.ssid.empty() ? "<隐藏网络>" : ap.ssid;
            list += column(ap.connected ? "已连接" : "-", STATE_WIDTH)
                + column(ap.encrypted ? "加密" : "开放x", SECURITY_WIDTH)
                + column(std::to_string(ap.signal) + " dBm", SIGNAL_WIDTH)
                + column(std::to_string(ap.freq) + " MHz", FREQUENCY_WIDTH)
                + ssid + "\n";
        }
    }
    mWifiList->setText(list.c_str());
}

/************************** 屏幕老化 **************************/

FactoryAging::FactoryAging(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    PBase::click(mRootView, [this](View&) {exit();});
    mTicker.setTick(3000);
    mTicker.setCallBack(std::bind(&FactoryAging::onTick, this, std::placeholders::_1));
}

void FactoryAging::onShow() {
    mRootView->getBackground()->setLevel(1);
    mTicker.startTick();
}

void FactoryAging::onHide() {
    mTicker.stopTick();
}

void FactoryAging::onTick(int64_t) {
    cdroid::Drawable* bg = mRootView->getBackground();
    int level = bg->getLevel();
    if (level < 5) {
        bg->setLevel(++level);
    } else {
        bg->setLevel(1);
    }
}
