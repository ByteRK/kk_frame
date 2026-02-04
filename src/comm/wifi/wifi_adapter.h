/*
 * @Author: xlc
 * @Email: 
 * @Date: 2026-01-30 19:48:35
 * @LastEditTime: 2026-02-04 16:53:54
 * @FilePath: /kk_frame/src/comm/wifi/wifi_adapter.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIFI_ADAPTER_H__
#define __WIFI_ADAPTER_H__

#define WIFI_ADAPTER_AS_RECYCLEVIEW 1

#include "thread_mgr.h"
#include "wifi_sta.h"
#include "template/singleton.h"
#include <stdint.h>
#if WIFI_ADAPTER_AS_RECYCLEVIEW
#include <widgetEx/recyclerview/recyclerview.h>
#else
#include <widget/adapter.h>
#endif

#pragma pack(1)
// WIFI列表
typedef struct tagWIFIAdapterData {
    uint8_t     locked;        // 需要密码 */
    int8_t      level;         // 信号强度 */
    uint8_t     conn_status;   // 连接状态 */
    std::string name;          // 名称 */
} WIFIAdapterData;
#pragma pack()


////////////////////////////////////////////////////////////////////////
class INetChange {
public:
    enum emStatus{NET_ERR = 0, NET_OK};
public:
    virtual void onNetChange(int status) = 0;
    virtual void onSignalChange(int ms) = 0;
};

/// @brief WIFI数据适配器
class WIFIAdapter : public ThreadTask,
#if WIFI_ADAPTER_AS_RECYCLEVIEW
    public RecyclerView::Adapter,
#else
    public cdroid::Adapter, 
#endif
    public Singleton<WIFIAdapter> {
    friend Singleton<WIFIAdapter>;
public:
    class Interface {
    public:
        virtual void       onData(const std::vector<WIFIAdapterData> &data);
        virtual void       onClickItem(ViewGroup *v, WIFIAdapterData *pdat);
        virtual ViewGroup* loadItemLayout(int type) = 0;
        virtual void       setItemLayout(ViewGroup *v, WIFIAdapterData *pdat) = 0;
    };
    enum emWifiStatus {
        WIFI_NULL = 0,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_DISCONNECTED,
    };

    /* 检测网络 */
    static void autoCheck();
    /* 网速级别 */
    static int  speedLevel(int ms);    
    /* 信号级别 */
    static int  signalLevel(int val);

    void setParent(Interface *parent);
    void start();
    void stop();
    void cancel();
    void onTick();
    void connecting(WIFIAdapterData *pdat);
    void regNetChange(INetChange *inetchange);

protected:
    WIFIAdapter();
#if WIFI_ADAPTER_AS_RECYCLEVIEW
    WIFIAdapterData          *getItem(int position);
    int                       getItemCount() override;
    int                       getItemViewType(int position) override;
    RecyclerView::ViewHolder *onCreateViewHolder(ViewGroup *parent, int viewType) override;
    void                      onBindViewHolder(RecyclerView::ViewHolder &holder, int position) override;
#else
    int   getCount() const override;
    void *getItem(int position) const override;
    View *getView(int position, View *convertView, ViewGroup *parent) override;
#endif
    virtual int  onTask(int id, void *data) override;
    virtual void onMain(int id, void *data) override;

protected:
    friend class SetWifi;
    INetChange                  *mNetChange;
    std::mutex                   mDataMutex;
    std::vector<WIFIAdapterData> mThreadWIFIData;
    std::vector<WIFIAdapterData> mShowWIFIData;
    int                          mWifiTaskID;
    int                          mNetCheckTaskId;
    int                          mPingFailCount;
    int                          mPingTimems;
    bool                         mLoadComplete;
    Interface                   *mInterface;
    bool                         mConnecting;
    int64_t                      mLastScanTime;
};

#endif // !__WIFI_ADAPTER_H__
