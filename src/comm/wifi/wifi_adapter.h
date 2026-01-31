/*
 * @Author: xlc
 * @Email: 
 * @Date: 2026-01-30 19:48:35
 * @LastEditTime: 2026-01-31 16:50:41
 * @FilePath: /kk_frame/src/class/wifi_adapter.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIFI_ADAPTER_H__
#define __WIFI_ADAPTER_H__

#include "thread_mgr.h"
#include <stdint.h>
#include <hv_wifi_sta.h>

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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// wifi data
class WIFIAdapter : public Adapter, public ThreadTask {
public:
    class Interface {
    public:
        virtual void onData(const std::vector<WIFIAdapterData> &data) {}
        virtual ViewGroup* loadLayout(const_string& res) { return nullptr; }
        virtual void onClickItem(ViewGroup *v, WIFIAdapterData *pdat){}        
    };
    enum emWifiStatus {
        WIFI_NULL = 0,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_DISCONNECTED,
    };

    static WIFIAdapter *ins();
    /* 检测网络 */
    static void autoCheck();
    /* 网速级别 */
    static int speedLevel(int ms);    
    /* 信号级别 */
    static int signalLevel(int val);

    void setParent(Interface *parent);
    void start();
    void stop();
    void cancel();
    void onTick();
    void connecting(WIFIAdapterData *pdat);
    void regNetChange(INetChange *inetchange);

protected:
    WIFIAdapter();
    virtual int   getCount() const;
    virtual void *getItem(int position) const;
    virtual View *getView(int position, View *convertView, ViewGroup *parent);
    virtual int   onTask(int id, void *data);
    virtual void  onMain(int id, void *data);

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
