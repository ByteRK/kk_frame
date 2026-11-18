/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-18 16:10:00
 * @LastEditTime: 2026-11-18 16:10:00
 * @FilePath: /kk_frame/src/app/page/core/item_switcher.h
 * @Description: 容器子项切换器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ITEM_SWITCHER_H__
#define __ITEM_SWITCHER_H__

#include "item_base.h"

#include <functional>
#include <map>
#include <utility>

#include "non_copyable.h"

/// @brief 容器子项切换器
/// @tparam ItemT 子项类型（需继承 ItemBase）
/// @tparam Args  子项构建参数（容器之外的额外参数）
/// @note 注册表按类型静态共享，缓存与当前项随实例回收；容器由切换器注入给子项
template <typename ItemT, typename... Args>
class ItemSwitcher : private NonCopyableNonMovable {
public:
    /// @brief 子项构建函数（容器由切换器提供，其余参数由宿主页面传入）
    using Creator = std::function<ItemT*(ViewGroup*, Args...)>;
    using ItemMap = std::map<int, ItemT*>;
    /// @brief 切换回调，id 为 0 时 item 为 nullptr
    using OnSwitchListener = std::function<void(int id, ItemT* item)>;

public:
    ItemSwitcher() = default;
    ~ItemSwitcher() { clear(); }

    /// @brief 注册子项（id 需唯一且非 0）
    static bool registerItem(int id, Creator creator) {
        if (id == 0 || !creator) {
            LOGE("item register failed, id=%d", id);
            return false;
        }
        if (creators().find(id) != creators().end()) {
            LOGE("item duplicated, id=%d", id);
            return false;
        }
        creators()[id] = std::move(creator);
        return true;
    }

    static bool hasItem(int id) {
        return creators().find(id) != creators().end();
    }

public:
    void init(ViewGroup* container) { mContainer = container; }

    ViewGroup* getContainer() const { return mContainer; }

    /// @brief 切换回调，用于同步菜单页等外围视图
    void setOnSwitchListener(OnSwitchListener l) { mOnSwitchListener = std::move(l); }

    /// @brief 切换显示子项
    /// @param id 子项标识，0 表示仅隐藏
    /// @return 实际显示的标识，0 表示已隐藏
    /// @note 未注册的标识会被忽略并保留当前状态
    int show(int id, Args... args) {
        if (id == mCurID) return mCurID;

        if (id != 0 && !hasItem(id)) {
            LOGE("item[%d] not registered", id);
            return mCurID;
        }
        if (id != 0 && mContainer == nullptr) {
            LOGE("item switcher container is null, id=%d", id);
            return mCurID;
        }

        // 先摘除当前项，保证容器内始终只有一项
        detachCurrent();
        if (id == 0) {
            notifySwitch();
            return 0;
        }

        ItemT* item = getItem(id, args...);
        if (item == nullptr) {
            notifySwitch();
            return 0;
        }

        item->callShow();
        mCurItem = item;
        mCurID = id;
        notifySwitch();
        return mCurID;
    }

    /// @brief 隐藏当前子项
    void hide() {
        if (mCurItem == nullptr) return;
        detachCurrent();
        notifySwitch();
    }

    /// @brief 隐藏并释放所有子项
    void clear() {
        hide();
        for (auto it = mItems.begin(); it != mItems.end(); ++it) {
            delete it->second;
        }
        mItems.clear();
    }

    int      getID() const { return mCurID; }                 // 当前子项标识
    ItemT*   getCurrent() const { return mCurItem; }          // 当前子项
    bool     isShown() const { return mCurItem != nullptr; }  // 是否有子项在显示
    ItemMap* getItems() { return &mItems; }                   // 已创建的子项

    /// @brief 获取子项，首次调用时创建
    /// @note args 仅首次创建时使用；未注册时返回 nullptr
    ItemT* getItem(int id, Args... args) {
        auto it = mItems.find(id);
        if (it != mItems.end()) return it->second;

        auto cit = creators().find(id);
        if (cit == creators().end()) {
            LOGE("item[%d] not registered", id);
            return nullptr;
        }

        ItemT* item = cit->second(mContainer, args...);
        FailFast(item == nullptr, "create item[%d] failed", id);
        if (item == nullptr) return nullptr;

        item->setContainer(mContainer);
        mItems[id] = item;
        return item;
    }

private:
    /// @brief 类型级注册表（每个 ItemT/Args 组合独立一份）
    static std::map<int, Creator>& creators() {
        static std::map<int, Creator> items;
        return items;
    }

    /// @brief 摘除当前项，不触发切换回调
    void detachCurrent() {
        if (mCurItem == nullptr) return;
        mCurItem->callHide();
        mCurItem = nullptr;
        mCurID = 0;
    }

    void notifySwitch() {
        if (mOnSwitchListener) mOnSwitchListener(mCurID, mCurItem);
    }

private:
    ViewGroup*       mContainer{ nullptr };     // 挂载容器
    ItemT*           mCurItem{ nullptr };       // 当前子项
    int              mCurID{ 0 };               // 当前子项标识
    ItemMap          mItems;                    // 已创建的子项[id]
    OnSwitchListener mOnSwitchListener;         // 切换回调
};

#endif // !__ITEM_SWITCHER_H__
