/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_system.cc
 * @Description: 系统信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_system.h"

#include "string_utils.h"
#include "time_utils.h"
#include "global_data.h"
#include "config_info.h"

#include <core/systemclock.h>

#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

FactorySystem::FactorySystem(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_system") {
    setExitBtn(AppRid::exit);

    mSystemInfo = PBase::get<TextView>(mRootView, AppRid::system_info);
    PBase::click(mRootView, AppRid::refresh, [this](View&) {setInfo();});

    // 静态信息仅采集一次，后续刷新只重读动态项
    buildStaticInfo();

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

void FactorySystem::buildStaticInfo() {
    // 主机与内核
    struct utsname uts = {};
    if (uname(&uts) == 0) {
        mStatic.hostName = uts.nodename;
        mStatic.kernel = std::string(uts.sysname) + " " + uts.release + " " + uts.machine;
    } else {
        mStatic.hostName = "-";
        mStatic.kernel = "-";
    }

    // CPU
    mStatic.cpuModel = getCpuModel();
    const long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    mStatic.cpuCount = cpuCount > 0 ? cpuCount : 0;

    // 内存与交换分区总量
    std::string memText;
    if (readTextFile("/proc/meminfo", memText)) {
        mStatic.memTotalKb = readFieldValue(memText, "MemTotal");
        mStatic.swapTotalKb = readFieldValue(memText, "SwapTotal");
    }

    // 进程信息
    mStatic.pid = getpid();

    // 分区总量（仅需容量，不必每次刷新都重复取）
    mStatic.rootDiskTotal = getDiskUsage("/").total;
    mStatic.dataDiskTotal = getDiskUsage(APP_RW_DIR).total;
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

    // 硬件信息（运行期不变，取自静态缓存）
    info += "主机名称: " + mStatic.hostName + "\n";
    info += "内核版本: " + mStatic.kernel + "\n";
    info += "CPU 型号: " + mStatic.cpuModel + "\n";
    info += StringUtils::format("CPU 核心: %ld\n", mStatic.cpuCount);

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

    // 内存信息（仅读取变化项）
    uint64_t availableKb = 0;
    uint64_t swapFreeKb = 0;
    std::string memText;
    if (readTextFile("/proc/meminfo", memText)) {
        availableKb = readFieldValue(memText, "MemAvailable");
        if (availableKb == 0) {
            // 老内核无 MemAvailable，退化为 Free + Buffers + Cached
            availableKb = readFieldValue(memText, "MemFree")
                + readFieldValue(memText, "Buffers")
                + readFieldValue(memText, "Cached");
        }
        swapFreeKb = readFieldValue(memText, "SwapFree");
    }

    if (mStatic.memTotalKb > 0) {
        const uint64_t usedKb = (availableKb < mStatic.memTotalKb)
            ? (mStatic.memTotalKb - availableKb) : 0;
        info += "内存总计: " + formatSize(mStatic.memTotalKb * 1024) + "\n";
        info += "内存可用: " + formatSize(availableKb * 1024) + "\n";
        info += StringUtils::format("内存占用: %.1f%%\n",
            usedKb * 100.0 / mStatic.memTotalKb);
    }

    if (mStatic.swapTotalKb > 0) {
        const uint64_t swapUsedKb = (swapFreeKb < mStatic.swapTotalKb)
            ? (mStatic.swapTotalKb - swapFreeKb) : 0;
        info += "交换分区: " + formatSize(swapUsedKb * 1024)
            + " / " + formatSize(mStatic.swapTotalKb * 1024) + "\n";
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
    info += StringUtils::format("进程 PID: %d\n", mStatic.pid);

    info += "\n";

    // 储存信息（总量取自静态缓存，仅查询可用容量）
    auto appendDisk = [&info](const char* name, uint64_t total, const DiskUsage& disk) {
        if (!disk.valid || total == 0) {
            info += StringUtils::format("%s: -\n", name);
            return;
        }
        info += StringUtils::format("%s: %s 可用 / %s (剩余 %.1f%%)\n",
            name,
            formatSize(disk.available).c_str(),
            formatSize(total).c_str(),
            disk.available * 100.0 / total);
    };
    appendDisk("根分区", mStatic.rootDiskTotal, getDiskUsage("/"));
    appendDisk("数据分区", mStatic.dataDiskTotal, getDiskUsage(APP_RW_DIR));

    mSystemInfo->setText(info.c_str());
}

FACTORY_ITEM_REGISTER(AppRid::to_system, FactorySystem);
