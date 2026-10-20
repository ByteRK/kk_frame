/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-19 10:00:00
 * @LastEditTime: 2026-10-20 09:36:43
 * @FilePath: /kk_frame/src/class/statistics_store.h
 * @Description: 数据统计存储模板
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
 *    数据以本地日期（yyyymmdd）为键稀疏存储：
 *      20261017 -> { ... }
 *      20261019 -> { ... }
 *    有记录的那天才存在条目，跨天由读取时计算，无需定时轮转。
 *
 *    派生类提供 Data 与 Traits 后实例化。
 *
 *    Traits 需提供：
 *      typedef Data
 *      static const char* filePath()                           // 主文件路径
 *      static const char* backupPath()                         // 备份文件路径
 *      static size_t      maxKeepDays()                        // 最大保留天数
 *      static uint32_t    checkInterval()                      // 自动保存检查间隔(ms)
 *      static uint32_t    backupInterval()                     // 备份间隔(ms)
 *      static void        toJson(const Data&, Json::Value&)    // 数据转 JSON
 *      static void        fromJson(Data&, const Json::Value&)  // JSON 转数据
 *      static void        merge(Data&, const Data&)            // 数据合并
 *
**/

#ifndef __STATISTICS_STORE_H__
#define __STATISTICS_STORE_H__

#include "auto_save.h"
#include "template/singleton.h"
#include "json_file_store.h"
#include "json_utils.h"
#include "time_utils.h"

#include <json/json.h>
#include <cdlog.h>

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <utility>
#include <vector>

/// @brief 数据统计存储模板
/// @tparam TraitsT 项目数据与策略
/// @note 日期由模板统一管理，Data 中不需要日期字段
template<typename TraitsT>
class StatisticsStore :
    public AutoSaveItem,
    public Singleton<StatisticsStore<TraitsT>> {
    friend Singleton<StatisticsStore<TraitsT>>;

public:
    typedef typename TraitsT::Data     Data;  // 数据类型
    typedef std::pair<int32_t, Data>   Entry; // first: 日期(yyyymmdd)

private:
    typedef std::map<int32_t, Data>           Buffer;     // 数据缓冲区，键为本地日期
    typedef typename Buffer::iterator         Iter;       // 缓冲区迭代器
    typedef typename Buffer::const_iterator   ConstIter;  // 常量迭代器

private:
    Buffer          mBuffer;       // 数据缓冲区
    bool            mHaveChange;   // 数据是否发生变化

private:
    StatisticsStore();
    ~StatisticsStore() override;

public:
    void            init();
    void            reset();
    bool            load();
    bool            save(bool isBackup = false) override;
    bool            haveChange() override;

public:
    size_t          size() const;

    /// @brief 获取所有有记录的日期数据
    /// @return (日期, 数据) 数组，按日期升序，自动过滤未来日期
    std::vector<Entry>   getAllDayData() const;

    /// @brief 获取指定日期的数据
    /// @param days_ago 从今天往前第几天，0 表示今天
    /// @return 数据指针，无记录或越界返回 nullptr
    const Data*          getDayData(uint16_t days_ago) const;

    /// @brief 获取连续N天的数据
    /// @param days_ago 起始于从今天往前第几天
    /// @param days_count 获取天数
    /// @return 连续数据数组，缺失日返回默认值，长度与 days_count 一致
    std::vector<Data>    getDayData(uint16_t days_ago, uint16_t days_count) const;

    /// @brief 添加今天的数据（在原基础上叠加）
    /// @param data 新增数据
    void                 addDayData(const Data& data);

private:
    static int32_t  today();
    static int32_t  dateBefore(uint32_t days_ago);
    bool            trim();
};

/************************* 实现 *************************/

template<typename TraitsT>
StatisticsStore<TraitsT>::StatisticsStore() :
    AutoSaveItem(TraitsT::checkInterval(), TraitsT::backupInterval()) {
    mHaveChange = false;
}

template<typename TraitsT>
StatisticsStore<TraitsT>::~StatisticsStore() { }

template<typename TraitsT>
void StatisticsStore<TraitsT>::init() {
    load();
    AutoSaveItem::init();
}

template<typename TraitsT>
void StatisticsStore<TraitsT>::reset() {
    JsonFileStore::remove(TraitsT::filePath(), TraitsT::backupPath());
    load();
    mHaveChange = true;
    LOGE("data factory reset. file=%s", TraitsT::filePath());
}

template<typename TraitsT>
bool StatisticsStore<TraitsT>::load() {
    if (TraitsT::maxKeepDays() == 0) {
        // 配置错误时拒绝加载，避免无边界增长与异常落盘
        LOGE("invalid maxKeepDays(0), skip load. file=%s", TraitsT::filePath());
        return false;
    }

    mBuffer.clear();

    Json::Value root;
    std::string loadingPath;
    const bool loaded = JsonFileStore::load(
        root, TraitsT::filePath(), TraitsT::backupPath(), &loadingPath,
        [](const Json::Value& value) {
        return value.isObject() && value["days"].isArray();
    });
    if (!loaded) {
        LOG(ERROR) << "no valid local data file. use null data. file=" << TraitsT::filePath();
        mHaveChange = true;
        return false;
    }

    if (!root.isObject()) { // 防御：理论上已被校验拦截
        LOGE("invalid root type. file=%s", loadingPath.c_str());
        mHaveChange = true;
        return false;
    }

    const Json::Value& days = root["days"];
    if (!days.isArray()) {
        LOGE("invalid days type. file=%s", loadingPath.c_str());
        mHaveChange = true;
        return false;
    }

    LOG(INFO) << "load local data. file=" << loadingPath;

    bool needSave = false;

    /**** 开始读取数据 ****/
    for (Json::ArrayIndex index = 0; index < days.size(); index++) {
        const Json::Value& item = days[index];
        if (!item.isObject()) {
            LOGW("invalid day item, index=%u, file=%s", index, loadingPath.c_str());
            needSave = true;
            continue;
        }

        const int32_t date = static_cast<int32_t>(JsonUtils::get<int>(item, "date", 0));
        if (date <= 0) {
            LOGW("invalid day date, index=%u, file=%s", index, loadingPath.c_str());
            needSave = true;
            continue;
        }

        Data data = Data();
        TraitsT::fromJson(data, item);

        Iter it = mBuffer.find(date);
        if (it == mBuffer.end()) mBuffer[date] = data;
        else {
            TraitsT::merge(it->second, data); // 重复日期合并
            needSave = true;
        }
    }
    /**** 结束读取数据 ****/

    // 有重复键合并或被裁剪时，需要重新落盘
    if (trim()) needSave = true;
    mHaveChange = needSave;
    return true;
}

template<typename TraitsT>
bool StatisticsStore<TraitsT>::save(bool isBackup) {
    Json::Value root;
    root["version"] = 2;
    root["max_days"] = static_cast<Json::UInt>(TraitsT::maxKeepDays());

    /**** 开始写入数据 ****/
    for (ConstIter it = mBuffer.begin(); it != mBuffer.end(); ++it) {
        Json::Value item;
        item["date"] = static_cast<Json::Int>(it->first);
        TraitsT::toJson(it->second, item);
        root["days"].append(item);
    }
    /**** 结束写入数据 ****/

    // 写盘成功后才复位标记，避免写失败后数据不再重试
    if (!JsonFileStore::save(root, TraitsT::filePath(), TraitsT::backupPath(), isBackup)) {
        LOGE("save failed. file=%s",
            isBackup ? TraitsT::backupPath() : TraitsT::filePath());
        return false;
    }

    mHaveChange = false;
    return true;
}

template<typename TraitsT>
bool StatisticsStore<TraitsT>::haveChange() {
    return mHaveChange;
}

template<typename TraitsT>
size_t StatisticsStore<TraitsT>::size() const {
    return mBuffer.size();
}

template<typename TraitsT>
std::vector<typename StatisticsStore<TraitsT>::Entry>
StatisticsStore<TraitsT>::getAllDayData() const {
    std::vector<Entry> result;
    result.reserve(mBuffer.size());

    const int32_t todayDate = today();
    for (ConstIter it = mBuffer.begin(); it != mBuffer.end(); ++it) {
        if (todayDate > 0 && it->first > todayDate) continue; // 过滤未来日期
        result.push_back(*it);
    }

    return result;
}

template<typename TraitsT>
const typename StatisticsStore<TraitsT>::Data*
StatisticsStore<TraitsT>::getDayData(uint16_t days_ago) const {
    if (days_ago >= TraitsT::maxKeepDays()) return nullptr;

    const int32_t date = dateBefore(days_ago);
    if (date <= 0) return nullptr;

    const int32_t todayDate = today();
    if (todayDate > 0 && date > todayDate) return nullptr;

    ConstIter it = mBuffer.find(date);
    return it == mBuffer.end() ? nullptr : &it->second;
}

template<typename TraitsT>
std::vector<typename StatisticsStore<TraitsT>::Data>
StatisticsStore<TraitsT>::getDayData(uint16_t days_ago, uint16_t days_count) const {
    std::vector<Data> result;

    const size_t keepDays = TraitsT::maxKeepDays();
    // 起始日已超出保留窗口时返回空，避免调用方把默认值当作真实数据
    if (keepDays == 0 || days_ago >= keepDays) return result;

    result.reserve(days_count);

    for (uint32_t i = 0; i < days_count; i++) {
        ConstIter it = mBuffer.find(dateBefore(static_cast<uint32_t>(days_ago) + i));
        // 缺失的日期补默认值，保证长度与下标一一对应
        result.push_back(it == mBuffer.end() ? Data() : it->second);
    }

    return result;
}

template<typename TraitsT>
void StatisticsStore<TraitsT>::addDayData(const Data& data) {
    if (TraitsT::maxKeepDays() == 0) {
        LOGE("invalid maxKeepDays(0), skip add. file=%s", TraitsT::filePath());
        return;
    }

    const int32_t date = today();
    if (date <= 0) {
        LOGE("addDayData failed: invalid date");
        return;
    }

    Iter it = mBuffer.find(date);
    if (it == mBuffer.end()) mBuffer[date] = data;
    else TraitsT::merge(it->second, data);

    trim();
    mHaveChange = true;
}

template<typename TraitsT>
int32_t StatisticsStore<TraitsT>::today() {
    const time_t now = TimeUtils::getTimeSec();
    if (now == static_cast<time_t>(-1)) return 0;
    return static_cast<int32_t>(TimeUtils::getDate(now));
}

template<typename TraitsT>
int32_t StatisticsStore<TraitsT>::dateBefore(uint32_t days_ago) {
    if (days_ago > 0xFFFF) return 0;
    return static_cast<int32_t>(TimeUtils::getDateDaysAgo(static_cast<uint16_t>(days_ago)));
}

template<typename TraitsT>
bool StatisticsStore<TraitsT>::trim() {
    const size_t keepDays = TraitsT::maxKeepDays();
    if (keepDays == 0 || mBuffer.empty()) return false;

    const int32_t keepFrom = dateBefore(static_cast<uint32_t>(keepDays - 1));
    if (keepFrom <= 0) return false; // 日期计算失败时不裁剪，避免误删

    bool removed = false;
    while (!mBuffer.empty() && mBuffer.begin()->first < keepFrom) {
        mBuffer.erase(mBuffer.begin());
        removed = true;
    }

    return removed;
}

#endif // !__STATISTICS_STORE_H__
