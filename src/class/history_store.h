/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-19 15:50:12
 * @LastEditTime: 2026-10-20 09:51:28
 * @FilePath: /kk_frame/src/class/history_store.h
 * @Description: 历史记录存储模板
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
 *    派生类提供 Data 与 Traits 后实例化。
 *
 *    Traits 需提供：
 *      typedef Data
 *      static const char* filePath()                          // 主文件路径
 *      static const char* backupPath()                        // 备份文件路径
 *      static size_t      maxCount()                          // 最大存储条数
 *      static uint32_t    checkInterval()                     // 自动保存检查间隔(ms)
 *      static uint32_t    backupInterval()                    // 备份间隔(ms)
 *      static void        toJson(const Data&, Json::Value&)   // 数据转 JSON
 *      static void        fromJson(Data&, const Json::Value&) // JSON 转数据
 *
**/

#ifndef __HISTORY_STORE_H__
#define __HISTORY_STORE_H__

#include "auto_save.h"
#include "template/singleton.h"
#include "json_file_store.h"
#include "json_utils.h"

#include <json/json.h>
#include <cdlog.h>

#include <stddef.h>
#include <stdint.h>
#include <iterator>
#include <list>
#include <vector>

/// @brief 历史记录存储模板
/// @tparam TraitsT 项目数据与策略
/// @note 新记录插入头部，超出容量后丢弃最旧记录
template<typename TraitsT>
class HistoryStore :
    public AutoSaveItem,
    public Singleton<HistoryStore<TraitsT> > {
    friend Singleton<HistoryStore<TraitsT> >;

public:
    typedef typename TraitsT::Data Data;  // 数据类型

private:
    typedef std::list<Data>                   Buffer;     // 数据缓冲区，头部为最新
    typedef typename Buffer::iterator         Iter;       // 缓冲区迭代器
    typedef typename Buffer::const_iterator   ConstIter;  // 常量迭代器

private:
    Buffer          mBuffer;     // 数据缓冲区，头部为最新
    bool            mHaveChange; // 数据是否发生变化

private:
    HistoryStore();
    ~HistoryStore() override;

public:
    void            init();
    void            reset();
    bool            load();
    bool            save(bool isBackup = false) override;
    bool            haveChange() override;

public:
    size_t          size() const;

    /// @brief 获取全部记录
    /// @note 指针在下一次增删之前有效（std::list 节点地址稳定）
    std::vector<const Data*> getHistory() const;

    /// @brief 获取指定位置的记录
    /// @param position 位置，0 为最新
    /// @return 记录指针，越界返回 nullptr
    const Data*              getHistory(int position) const;

    /// @brief 获取指定范围的记录
    /// @param start 起始位置（含）
    /// @param end 结束位置（含）
    /// @return 记录指针数组，越界部分自动截断
    std::vector<const Data*> getHistory(int start, int end) const;

    /// @brief 添加记录（插入头部）
    void                     addHistory(const Data& data);

    /// @brief 删除指定位置的记录
    void                     delHistory(int position);
};

/************************* 实现 *************************/

template<typename TraitsT>
HistoryStore<TraitsT>::HistoryStore() :
    AutoSaveItem(TraitsT::checkInterval(), TraitsT::backupInterval()) {
    mHaveChange = false;
}

template<typename TraitsT>
HistoryStore<TraitsT>::~HistoryStore() { }

template<typename TraitsT>
void HistoryStore<TraitsT>::init() {
    load();
    AutoSaveItem::init();
}

template<typename TraitsT>
void HistoryStore<TraitsT>::reset() {
    JsonFileStore::remove(TraitsT::filePath(), TraitsT::backupPath());
    load();
    mHaveChange = true;
    LOGE("data factory reset. file=%s", TraitsT::filePath());
}

template<typename TraitsT>
bool HistoryStore<TraitsT>::load() {
    const size_t limit = TraitsT::maxCount();
    if (limit == 0) {
        // 配置错误时不覆盖旧数据，避免静默清空历史
        LOGE("invalid maxCount(0), skip load. file=%s", TraitsT::filePath());
        return false;
    }

    mBuffer.clear();

    Json::Value root;
    std::string loadingPath;
    const bool loaded = JsonFileStore::load(
        root, TraitsT::filePath(), TraitsT::backupPath(), &loadingPath,
        [](const Json::Value& value) { return value.isArray(); });
    if (!loaded) {
        LOG(ERROR) << "no valid local data file. use null data. file=" << TraitsT::filePath();
        mHaveChange = true;
        return false;
    }

    if (!root.isArray()) { // 防御：理论上已被校验拦截
        LOGE("invalid root type. file=%s", loadingPath.c_str());
        mHaveChange = true;
        return false;
    }

    LOG(INFO) << "load local data. file=" << loadingPath;

    bool needSave = false;

    /**** 开始读取数据 ****/
    for (Json::ArrayIndex index = 0; index < root.size(); index++) {
        const Json::Value& item = root[index];
        if (!item.isObject()) {
            LOGW("invalid history item, index=%u, file=%s", index, loadingPath.c_str());
            needSave = true;
            continue;
        }

        if (mBuffer.size() >= limit) { // 超出容量，丢弃后续旧数据
            needSave = true;
            break;
        }

        Data data = Data();
        TraitsT::fromJson(data, item);
        mBuffer.push_back(data);
    }
    /**** 结束读取数据 ****/

    mHaveChange = needSave;
    return true;
}

template<typename TraitsT>
bool HistoryStore<TraitsT>::save(bool isBackup) {
    Json::Value root(Json::arrayValue);

    /**** 开始写入数据 ****/
    for (ConstIter it = mBuffer.begin(); it != mBuffer.end(); ++it) {
        Json::Value item;
        TraitsT::toJson(*it, item);
        root.append(item);
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
bool HistoryStore<TraitsT>::haveChange() {
    return mHaveChange;
}

template<typename TraitsT>
size_t HistoryStore<TraitsT>::size() const {
    return mBuffer.size();
}

template<typename TraitsT>
std::vector<const typename HistoryStore<TraitsT>::Data*>
HistoryStore<TraitsT>::getHistory() const {
    std::vector<const Data*> result;
    result.reserve(mBuffer.size());

    for (ConstIter it = mBuffer.begin(); it != mBuffer.end(); ++it)
        result.push_back(&(*it));

    return result;
}

template<typename TraitsT>
const typename HistoryStore<TraitsT>::Data*
HistoryStore<TraitsT>::getHistory(int position) const {
    if (position < 0 || static_cast<size_t>(position) >= mBuffer.size())
        return nullptr;

    ConstIter it = mBuffer.begin();
    std::advance(it, position);
    return &(*it);
}

template<typename TraitsT>
std::vector<const typename HistoryStore<TraitsT>::Data*>
HistoryStore<TraitsT>::getHistory(int start, int end) const {
    std::vector<const Data*> result;
    if (start < 0 || end < start) return result;
    if (static_cast<size_t>(start) >= mBuffer.size()) return result;

    ConstIter it = mBuffer.begin();
    std::advance(it, start);

    for (int position = start; position <= end && it != mBuffer.end(); position++, ++it)
        result.push_back(&(*it));

    return result;
}

template<typename TraitsT>
void HistoryStore<TraitsT>::addHistory(const Data& data) {
    const size_t limit = TraitsT::maxCount();
    if (limit == 0) {
        LOGE("invalid maxCount(0), skip add. file=%s", TraitsT::filePath());
        return;
    }

    mBuffer.push_front(data);

    while (mBuffer.size() > limit) mBuffer.pop_back();

    mHaveChange = true;
}

template<typename TraitsT>
void HistoryStore<TraitsT>::delHistory(int position) {
    if (position < 0 || static_cast<size_t>(position) >= mBuffer.size())
        return;

    Iter it = mBuffer.begin();
    std::advance(it, position);
    mBuffer.erase(it);

    mHaveChange = true;
}

#endif // !__HISTORY_STORE_H__
