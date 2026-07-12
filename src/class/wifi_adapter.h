/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-28 11:40:38
 * @LastEditTime: 2026-06-30 14:44:47
 * @FilePath: /kk_frame/src/class/wifi_adapter.h
 * @Description: WIFI适配器（UI用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIFI_ADAPTER_H__
#define __WIFI_ADAPTER_H__

#include "quick_define.h"

#if ENABLED(WIFI) || defined(__VSCODE__)

#include "wifi_mgr.h"
#include "template/singleton.h"
#include <view/view.h>
#include <stdint.h>
#include <memory>

#ifndef WIFI_ADAPTER_CLICK_MATCH_BSSID
#define WIFI_ADAPTER_CLICK_MATCH_BSSID 0
#endif

/////////////////////////////////////////////////////////////////////////////////////

/// @brief WIFI适配器基类
class B_WifiAdapter :public WifiMgr::WiFiListener {
public:
    typedef enum {
        DISPLAY_TYPE_REMOVE,  // 移除出队列
        DISPLAY_TYPE_KEEP,    // 保留
        DISPLAY_TYPE_TOP,     // 移到最前
    } CONNECTED_DISPLAY;

    class Interface {
        friend class B_WifiAdapter;
    public:
        virtual ~Interface() = default;
        virtual void   flushEnd() { };
        virtual void   onClickItem(View* v, WifiHal::ApInfo* apInfo) { };
        virtual View*  loadItemLayout(int type) = 0;
        virtual void   setItemLayout(int position, View* v, WifiHal::ApInfo* apInfo) = 0;

    private:
        std::shared_ptr<int> mLifetime{ std::make_shared<int>(0) };
    };

public:
    // 设置适配器的页面接口，传入 nullptr 解除绑定
    void setParent(Interface* parent);

    // 设置已连接的WIFI的显示方式
    void setConnectedDisplay(CONNECTED_DISPLAY type);

    static int speedLevel(int ms);    // 网速级别
    static int signalLevel(int val);  // 信号级别

protected:
    /// @brief Wi-Fi 状态变化回调，关闭时清空 AP 缓存和展示列表
    virtual void onStateChanged() override;

    /// @brief 扫描结果回调，从 WifiMgr 刷新原始 AP 缓存并更新展示列表
    virtual void onScanResult() override;

    /// @brief 通知具体 Adapter 展示列表已经变化
    virtual void notifyApListChanged() = 0;

    /// @brief 刷新展示列表
    /// @param reloadSource true 时通过 g_wifi->getAps 更新原始缓存；false 时复用现有缓存
    void refreshApList(bool reloadSource);

    /// @brief 根据已连接项显示策略，从原始缓存生成展示列表
    void processApList();

    /// @brief 清空原始缓存和展示列表
    /// @param sourceInitialized 清空后是否将原始缓存视为已初始化
    void clearApList(bool sourceInitialized);

    /// @brief 依次通知页面接口和具体 Adapter 刷新展示
    void dispatchApListChanged();

    /// @brief 获取仍存活的页面接口；页面销毁后自动使旧绑定失效。
    Interface* getParent();

protected:
    std::vector<WifiHal::ApInfo> mSourceApInfoList;                          // 未经处理的原始 AP 缓存
    std::vector<WifiHal::ApInfo> mApInfoList;                                // 提供给 Adapter 展示的 AP 列表
    Interface*                   mInterface{ nullptr };                      // 当前绑定的页面接口
    std::weak_ptr<int>           mInterfaceLifetime;                        // 页面接口生命周期标记
    CONNECTED_DISPLAY            mConnectedItemDisplay{ DISPLAY_TYPE_KEEP }; // 已连接项显示策略
    bool                         mSourceApListInitialized{ false };          // 原始 AP 缓存是否已初始化
};

/////////////////////////////////////////////////////////////////////////////////////
#include <widget/adapter.h>

/// @brief WIFI适配器(列表版)
class WifiAdapter : public cdroid::Adapter,
    public B_WifiAdapter,
    public Singleton<WifiAdapter> {
    friend Singleton<WifiAdapter>;
protected:
    WifiAdapter();
    ~WifiAdapter();

public:
    void  clear();

protected:
    void  onStateChanged() override;
    void  notifyApListChanged() override;

    int   getCount() const override;
    void* getItem(int position) const override;
    int   getItemViewType(int position) const override;
    int   getViewTypeCount() const override;
    View* getView(int position, View* convertView, ViewGroup* parent) override;
};

/////////////////////////////////////////////////////////////////////////////////////
#include <widgetEx/recyclerview/recyclerview.h>

/// @brief WIFI适配器(RecyclerView版)
class WifiRecycleAdapter : public RecyclerView::Adapter,
    public B_WifiAdapter,
    public Singleton<WifiRecycleAdapter> {
    friend Singleton<WifiRecycleAdapter>;
protected:
    WifiRecycleAdapter();
    ~WifiRecycleAdapter();

public:
    void  clear();

protected:
    void                      onStateChanged() override;
    void                      notifyApListChanged() override;

    WifiHal::ApInfo*          getItem(int position);
    int                       getItemCount() override;
    int                       getItemViewType(int position) override;
    RecyclerView::ViewHolder* onCreateViewHolder(ViewGroup* parent, int viewType) override;
    void                      onBindViewHolder(RecyclerView::ViewHolder& holder, int position) override;

private:
    void                      onClickItem(View* v, RecyclerView::ViewHolder* holder);
};

#endif // !ENABLED(WIFI)

#endif // !__WIFI_ADAPTER_H__
