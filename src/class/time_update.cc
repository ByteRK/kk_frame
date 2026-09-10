/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-10 17:09:58
 * @LastEditTime: 2026-09-10 17:18:03
 * @FilePath: /kk_frame/src/class/time_update.cc
 * @Description: 时间文本刷新器 - 统一托管并按秒刷新显示时间的 TextView
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "time_update.h"
#include "time_utils.h"

#include <cdlog.h>

const char* TimeUpdate::kDefaultFmt = "%H:%M";
const int64_t TimeUpdate::kTickIntervalMs;

/// @brief 时间文本刷新器构造
TimeUpdate::TimeUpdate() { }

/// @brief 时间文本刷新器析构
TimeUpdate::~TimeUpdate() {
    clear();
}

/// @brief 注册视图，上下午使用同一时间格式
/// @param view 目标视图
/// @param fmt 时间格式
/// @return 状态
bool TimeUpdate::add(TextView* view, const std::string& fmt /* = kDefaultFmt */) {
    Item spec;
    spec.fmt = fmt;
    return addItem(view, spec);
}

/// @brief 注册视图，上下午使用各自的时间格式
/// @param view 目标视图
/// @param fmtAM 上午时间格式
/// @param fmtPM 下午时间格式
/// @return 状态
bool TimeUpdate::add(TextView* view, const std::string& fmtAM, const std::string& fmtPM) {
    Item spec;
    spec.fmtAM = fmtAM;
    spec.fmtPM = fmtPM;
    return addItem(view, spec);
}

/// @brief 注册视图，文本由外部提供者生成
/// @param view 目标视图
/// @param provider 文本提供者
/// @return 状态
bool TimeUpdate::add(TextView* view, const TextProvider& provider) {
    Item spec;
    spec.provider = provider;
    return addItem(view, spec);
}

/// @brief 校验视图配置是否可用
/// @param item 视图配置
/// @return 状态
bool TimeUpdate::isValidItem(const TimeUpdate::Item& item) {
    if (item.provider) return true;
    if (!item.fmt.empty()) return true;
    return (!item.fmtAM.empty() && !item.fmtPM.empty());
}

/// @brief 注册/更新视图
/// @param view 目标视图
/// @param spec 视图配置，其中 view 字段会被忽略
/// @return 状态
bool TimeUpdate::addItem(TextView* view, const Item& spec) {
    if (view == nullptr) {
        LOGE("TimeUpdate add failed: view is nullptr");
        return false;
    }

    // 自定义提供者、统一格式、上下午格式至少要有一组可用
    if (!isValidItem(spec)) {
        LOGE("TimeUpdate add failed: invalid format/provider, view=%p", view);
        return false;
    }

    Item* item = find(view);
    if (item == nullptr) {
        Item record = spec;
        record.view = view;
        mItems.push_back(record);
        item = &mItems.back();

        LOGD("TimeUpdate add: view=%p, format=%s, count=%zu",
            view, spec.provider ? "provider" : (spec.fmt.empty() ? "AM/PM" : spec.fmt.c_str()),
            mItems.size());

        // 首个视图注册时，自行注册 TickMgr
        startTickIfNeeded();
    } else {
        LOGD("TimeUpdate add: view already exists, update config, view=%p", view);
        item->fmt = spec.fmt;
        item->fmtAM = spec.fmtAM;
        item->fmtPM = spec.fmtPM;
        item->provider = spec.provider;
    }

    // 注册或更新配置后立即刷新一次，保证视图立刻显示正确内容
    updateItem(*item, true);
    return true;
}

/// @brief 注销视图
/// @param view 目标视图
/// @return 状态
bool TimeUpdate::remove(TextView* view) {
    if (view == nullptr) {
        LOGE("TimeUpdate remove failed: view is nullptr");
        return false;
    }

    for (std::vector<Item>::iterator it = mItems.begin(); it != mItems.end(); ++it) {
        if (it->view != view) continue;
        mItems.erase(it);

        LOGD("TimeUpdate remove: view=%p, count=%zu", view, mItems.size());

        // 全部视图移除后，自动取消注册 TickMgr
        stopTickIfEmpty();
        return true;
    }

    LOGE("TimeUpdate remove failed: view not found, view=%p", view);
    return false;
}

/// @brief 更新已注册视图的时间格式
/// @param view 目标视图
/// @param fmt 时间格式
/// @return 状态
bool TimeUpdate::setFormat(TextView* view, const std::string& fmt) {
    if (fmt.empty()) {
        LOGE("TimeUpdate setFormat failed: invalid format");
        return false;
    }

    Item* item = find(view);
    if (item == nullptr) {
        LOGE("TimeUpdate setFormat failed: view not found, view=%p", view);
        return false;
    }

    item->fmt = fmt;
    item->fmtAM.clear();
    item->fmtPM.clear();
    item->provider = nullptr;
    return updateItem(*item, true);
}

/// @brief 更新已注册视图的时间格式，上下午使用各自的时间格式
/// @param view 目标视图
/// @param fmtAM 上午时间格式
/// @param fmtPM 下午时间格式
/// @return 状态
bool TimeUpdate::setFormat(TextView* view, const std::string& fmtAM, const std::string& fmtPM) {
    if (fmtAM.empty() || fmtPM.empty()) {
        LOGE("TimeUpdate setFormat failed: invalid format");
        return false;
    }

    Item* item = find(view);
    if (item == nullptr) {
        LOGE("TimeUpdate setFormat failed: view not found, view=%p", view);
        return false;
    }

    item->fmt.clear();
    item->fmtAM = fmtAM;
    item->fmtPM = fmtPM;
    item->provider = nullptr;
    return updateItem(*item, true);
}

/// @brief 更新已注册视图的文本提供者
/// @param view 目标视图
/// @param provider 文本提供者
/// @return 状态
bool TimeUpdate::setProvider(TextView* view, const TextProvider& provider) {
    if (!provider) {
        LOGE("TimeUpdate setProvider failed: provider is empty");
        return false;
    }

    Item* item = find(view);
    if (item == nullptr) {
        LOGE("TimeUpdate setProvider failed: view not found, view=%p", view);
        return false;
    }

    item->fmt.clear();
    item->fmtAM.clear();
    item->fmtPM.clear();
    item->provider = provider;
    return updateItem(*item, true);
}

/// @brief 立即刷新全部视图
void TimeUpdate::refresh() {
    tickAll(true);
}

/// @brief 清空全部视图
void TimeUpdate::clear() {
    // 列表为空时说明 TickMgr 早已注销，直接返回可避免析构阶段再次访问 TickMgr
    if (mItems.empty()) return;

    LOGD("TimeUpdate clear: count=%zu", mItems.size());
    mItems.clear();
    stopTickIfEmpty();
}

/// @brief 已托管视图数量
/// @return 数量
size_t TimeUpdate::size() const {
    return mItems.size();
}

/// @brief 视图是否已被托管
/// @param view 目标视图
/// @return 状态
bool TimeUpdate::contains(const TextView* view) const {
    return find(view) != nullptr;
}

/// @brief 是否已注册 TickMgr
/// @return 状态
bool TimeUpdate::isRunning() const {
    return g_tick->hasTick(this);
}

/// @brief Tick 回调
/// @param nowMs 当前时间戳（毫秒）
void TimeUpdate::onTick(int64_t nowMs) {
    tickAll(false);
}

/// @brief 刷新全部视图
/// @param force 是否忽略去重缓存强制写入
void TimeUpdate::tickAll(bool force) {
    // 按下标遍历：刷新过程中若发生注销(如 TextWatcher 回调)也不会导致迭代器失效，
    // 被顺延跳过的视图会在下一次 Tick 中补齐
    for (size_t i = 0; i < mItems.size(); ++i) {
        updateItem(mItems[i], force);
    }
}

/// @brief 刷新单个视图
/// @param item 视图记录
/// @param force 是否忽略去重缓存强制写入
/// @return 是否写入了新文本
bool TimeUpdate::updateItem(Item& item, bool force) {
    if (item.view == nullptr) return false;

    std::string text;
    if (item.provider) {
        text = item.provider();
    } else if (!item.fmtAM.empty() && !item.fmtPM.empty()) {
        text = TimeUtils::getTimeFmtStrAP(item.fmtAM.c_str(), item.fmtPM.c_str());
    } else if (!item.fmt.empty()) {
        text = TimeUtils::getTimeFmtStr(item.fmt.c_str());
    }

    // 生成失败时保留原有内容，避免清空视图
    if (text.empty()) return false;
    // 文本未变化时跳过写入，避免无意义的 setText/invalidate
    if (!force && text == item.lastText) return false;

    item.lastText = text;
    item.view->setText(text);
    return true;
}

/// @brief 首个视图注册时注册 TickMgr
void TimeUpdate::startTickIfNeeded() {
    if (isRunning()) return;

    setTick(kTickIntervalMs);
    // 首次调度对齐到下一个整秒，使刷新与时间跳变同步
    startTick(delayToNextSecond());
}

/// @brief 全部视图移除后取消 TickMgr 注册
void TimeUpdate::stopTickIfEmpty() {
    if (!mItems.empty()) return;
    stopTick();
}

/// @brief 计算到下一个整秒的延迟
/// @return 延迟毫秒数
int64_t TimeUpdate::delayToNextSecond() const {
    const int64_t nowMs = TimeUtils::getTimeMSec();
    if (nowMs <= 0) return kTickIntervalMs; // 获取失败时退化为整周期延迟

    const int64_t offset = nowMs % kTickIntervalMs;
    return kTickIntervalMs - offset;
}

/// @brief 查找视图记录
/// @param view 目标视图
/// @return 记录指针
TimeUpdate::Item* TimeUpdate::find(const TextView* view) {
    if (view == nullptr) return nullptr;
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].view == view) return &mItems[i];
    }
    return nullptr;
}

/// @brief 查找视图记录
/// @param view 目标视图
/// @return 记录指针
const TimeUpdate::Item* TimeUpdate::find(const TextView* view) const {
    if (view == nullptr) return nullptr;
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].view == view) return &mItems[i];
    }
    return nullptr;
}
