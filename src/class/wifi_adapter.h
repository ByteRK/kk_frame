/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-28 11:40:38
 * @LastEditTime: 2026-02-28 16:36:15
 * @FilePath: /kk_frame/src/class/wifi_adapter.h
 * @Description: WIFI适配器（UI用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIFI_ADAPTER_H__
#define __WIFI_ADAPTER_H__

#include "src/common.h"
#if ENABLED(WIFI) || defined(__VSCODE__)

#include "wifi_mgr.h"
#include "template/singleton.h"
#include <view/view.h>
#include <stdint.h>

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
    public:
        virtual void   flushEnd() { };
        virtual void   onClickItem(View* v, WifiHal::ApInfo* apInfo) { };
        virtual View*  loadItemLayout(int type) = 0;
        virtual void   setItemLayout(int position, View* v, WifiHal::ApInfo* apInfo) = 0;
    };

public:
    // 获取适配器
    void setParent(Interface* parent);

    // 设置已连接的WIFI的显示方式
    void setConnectedDisplay(CONNECTED_DISPLAY type);

    static int speedLevel(int ms);    // 网速级别
    static int signalLevel(int val);  // 信号级别

protected:
    virtual void onStateChanged() override;
    virtual void onScanResult() override;

protected:
    std::vector<WifiHal::ApInfo> mApInfoList;
    Interface*                   mInterface{ nullptr };
    CONNECTED_DISPLAY            mConnectedItemDisplay{ DISPLAY_TYPE_KEEP };
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
    void  onScanResult() override;

    int   getCount() const override;
    void* getItem(int position) const override;
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
    void                      onScanResult() override;

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
