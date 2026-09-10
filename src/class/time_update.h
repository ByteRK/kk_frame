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

#ifndef __TIME_UPDATE_H__
#define __TIME_UPDATE_H__

#include <stdint.h>
#include <stddef.h>
#include <functional>
#include <string>
#include <vector>

#include <widget/textview.h>

#include "tick_mgr.h"
#include "template/singleton.h"

/// @brief 时间文本刷新器
/// @note 托管一批显示当前时间的 TextView，统一按秒刷新，避免各处自行维护定时器
/// @note 首个视图注册时自动注册 TickMgr，最后一个视图移除后自动取消注册
/// @note 不持有视图所有权，视图销毁前必须先调用 remove 注销
class TimeUpdate : public TickMgr::ITickClass,
    public Singleton<TimeUpdate> {
    friend Singleton<TimeUpdate>;

public:
    /// @brief 默认时间格式 [%H:%M]
    static const char* kDefaultFmt;
    /// @brief 刷新周期（毫秒）
    static const int64_t kTickIntervalMs = 1000;

    /// @brief 自定义文本提供者
    /// @note 用于表达 strftime 无法描述的内容，例如带中文星期的日期
    typedef std::function<std::string()> TextProvider;

private:
    /// @brief 托管视图记录
    struct Item {
        TextView*        view{ nullptr };    // 目标视图
        std::string      fmt{};              // 统一时间格式
        std::string      fmtAM{};            // 上午时间格式
        std::string      fmtPM{};            // 下午时间格式
        TextProvider     provider{};         // 自定义文本提供者，非空时优先于格式
        std::string      lastText{};         // 上次刷新文本，用于跳过无变化的刷新
    };

protected:
    TimeUpdate();
    ~TimeUpdate();

public:
    /// @brief 注册视图，上下午使用同一时间格式，注册后立即刷新一次
    /// @param view 目标视图
    /// @param fmt 时间格式，参考 TimeUtils::getTimeFmtStr，例如 "%H:%M"
    /// @return true 注册成功（含已注册视图更新格式），false 参数非法
    bool add(TextView* view, const std::string& fmt = kDefaultFmt);

    /// @brief 注册视图，上下午使用各自的时间格式，注册后立即刷新一次
    /// @param view 目标视图
    /// @param fmtAM 上午时间格式，小时位请使用 %I 格式化为 12 小时制
    /// @param fmtPM 下午时间格式，小时位请使用 %I 格式化为 12 小时制
    /// @return true 注册成功（含已注册视图更新格式），false 参数非法
    bool add(TextView* view, const std::string& fmtAM, const std::string& fmtPM);

    /// @brief 注册视图，文本由外部提供者生成，注册后立即刷新一次
    /// @param view 目标视图
    /// @param provider 文本提供者，返回要写入视图的完整文本
    /// @return true 注册成功（含已注册视图更新提供者），false 参数非法
    /// @note 用于表达 strftime 无法描述的内容，例如带中文星期的日期
    bool add(TextView* view, const TextProvider& provider);

    /// @brief 注销视图，全部视图注销后自动取消 TickMgr 注册
    /// @param view 目标视图
    /// @return true 注销成功，false 未注册或参数非法
    bool remove(TextView* view);

    /// @brief 更新已注册视图的时间格式并立即刷新
    /// @param view 目标视图
    /// @param fmt 时间格式，参考 TimeUtils::getTimeFmtStr
    /// @return true 更新成功，false 未注册或参数非法
    bool setFormat(TextView* view, const std::string& fmt);

    /// @brief 更新已注册视图的时间格式并立即刷新，上下午使用各自的时间格式
    /// @param view 目标视图
    /// @param fmtAM 上午时间格式，小时位请使用 %I 格式化为 12 小时制
    /// @param fmtPM 下午时间格式，小时位请使用 %I 格式化为 12 小时制
    /// @return true 更新成功，false 未注册或参数非法
    bool setFormat(TextView* view, const std::string& fmtAM, const std::string& fmtPM);

    /// @brief 更新已注册视图的文本提供者并立即刷新
    /// @param view 目标视图
    /// @param provider 文本提供者，返回要写入视图的完整文本
    /// @return true 更新成功，false 未注册或参数非法
    bool setProvider(TextView* view, const TextProvider& provider);
    /// @brief 立即刷新全部视图
    /// @note 忽略文本去重缓存强制写入，可用于时区或系统时间变更后的同步
    void refresh();

    /// @brief 清空全部视图并取消 TickMgr 注册
    void clear();

    /// @brief 已托管视图数量
    /// @return 数量
    size_t size() const;

    /// @brief 视图是否已被托管
    /// @param view 目标视图
    /// @return true or false
    bool contains(const TextView* view) const;

    /// @brief 是否已注册 TickMgr
    /// @return true or false
    bool isRunning() const;

protected:
    void onTick(int64_t nowMs) override;

private:
    /// @brief 注册/更新视图
    /// @param view 目标视图
    /// @param spec 视图配置，其中 view 字段会被忽略
    /// @return true or false
    bool addItem(TextView* view, const Item& spec);

    /// @brief 校验视图配置是否可用（格式与提供者至少有一项）
    /// @param item 视图配置
    /// @return true or false
    static bool isValidItem(const Item& item);

    /// @brief 刷新全部视图
    /// @param force 是否忽略去重缓存强制写入
    void tickAll(bool force);
    /// @brief 刷新单个视图
    /// @param item 视图记录
    /// @param force 是否忽略去重缓存强制写入
    /// @return true 已写入新文本，false 未写入
    bool updateItem(Item& item, bool force);

    /// @brief 首个视图注册时注册 TickMgr
    void startTickIfNeeded();
    /// @brief 全部视图移除后取消 TickMgr 注册
    void stopTickIfEmpty();
    /// @brief 计算到下一个整秒的延迟，用于让刷新与整秒对齐
    /// @return 延迟毫秒数
    int64_t delayToNextSecond() const;

    /// @brief 查找视图记录
    /// @param view 目标视图
    /// @return 记录指针，未找到返回 nullptr
    Item* find(const TextView* view);
    /// @brief 查找视图记录
    /// @param view 目标视图
    /// @return 记录指针，未找到返回 nullptr
    const Item* find(const TextView* view) const;

private:
    std::vector<Item> mItems;   // 已托管视图
};

#endif // !__TIME_UPDATE_H__
