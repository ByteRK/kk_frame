#ifndef __WIFI_ADAPTER_H_
#define __WIFI_ADAPTER_H_
// #include <homewindow.h>
#include <hv_wifi_sta.h>
#include <common.h>
#include <comm_class.h>
#include <core/handler.h>

// WIFI列表
typedef struct tagWIFIAdapterData {
    uchar       locked : 1;      // 需要密码
    int         quality;         // 质量
    int         signalLevel;     // 信号强度
    int         level;           // 信号强度
    uchar       conn_status : 2; // 连接状态
    std::string name;            // 名称
} WIFIAdapterData;

// WIFI 连接信息
typedef struct {
    uchar       locked : 1;      // 需要密码
    std::string name;            // 名称
    std::string key;
} WIFIConnectData;

// // wifi 信息
// typedef struct{
//     bool scanStatus;
    
//     int connStatus;
//     WIFIAdapterData connData;
// }WifiData;

enum emWifiStatus {
    WIFI_CONNECTING,        // 连接中
    WIFI_CONNECTED,         // 已连接
    WIFI_DISCONNECTED,      // 未连接
};


/////////////////////////////////////////////////////////////////////////////

//     recyclerView  的 适配器
class wifiRecycAdapter : public RecyclerView::Adapter,RecyclerView::AdapterDataObserver{
public:
    typedef std::function<void(int eventAction)> connClickCallback;
private:

    static constexpr int VIEW_TYPE_HEADER   = 0;
    static constexpr int VIEW_TYPE_ITEM     = 1;

    ViewGroup   *mParentView;
    RecyclerView *mRecyclerView;

    bool        mIsHeaderView = false;
    connClickCallback mConnectViewClickCallback;
    std::vector<WIFIAdapterData> mWifi;
public:
    class ViewHolder:public RecyclerView::ViewHolder {
    public:
        ViewGroup* viewGroup;
        ViewHolder(View* itemView):RecyclerView::ViewHolder(itemView){
            viewGroup =(ViewGroup*)itemView;
        }
    };

    wifiRecycAdapter(ViewGroup *parent,RecyclerView *recyclerView);
    ~wifiRecycAdapter(){LOGE("~RecycAdapter()");}

    RecyclerView::ViewHolder*onCreateViewHolder(ViewGroup* parent, int viewType)override;
    void onBindViewHolder(RecyclerView::ViewHolder& holder, int position)override;
    int getItemCount()override;
    int getItemViewType(int position) override;

    void notifyData();
    void setHeaderView(bool isHeaderView);
    void setConnectViewClickCallback(connClickCallback callback);
    
};


/////////////////////////////////////////////////////////////////////////////

class WIFIScanThread :public ThreadTask{
    virtual int  onTask(void *data);
    virtual void onMain(void *data);
};

class WIFIConnectThread :public ThreadTask{
public:
    const std::string name;
    const std::string key;
private:
    virtual int  onTask(void *data);
    virtual void onMain(void *data);
};

class WIFIMgr:public MessageHandler{
protected:
    enum {
        MSG_DELAY_CONN,         // 延迟 连接wifi
        MSG_DELAY_DISCONN,      // 延迟 断开连接wifi
    };
private:
    WIFIConnectData mConnData;
    std::vector<WIFIAdapterData> sWifiScanData;

    bool    mIsStartScan;
    int     mIsConnStatus;
protected:
    WIFIMgr();
    ~WIFIMgr();

    void handleMessage(Message& message)override;
public:
    static WIFIMgr *ins() {
        static WIFIMgr ins;
        return &ins;
    }
    void init();
    void scanWifi();
    void delayConnWifi(WIFIConnectData connWifiData,int time);
    void disConnWifi();

    void scanEnd();
    void setConnStatus(int status);
    int  connStatus();

    void setWifiEnable(bool isEnable);
};

extern std::vector<WIFIAdapterData> sWifiData;
extern WIFIConnectData connWifiData;
#endif