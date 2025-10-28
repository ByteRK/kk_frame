#include <wifi_adapter.h>
#include <homewindow.h>
#include "input.h"
#ifdef TUYA_OS
#include "tuya_os_mgr.h"
#endif

std::vector<WIFIAdapterData> sWifiData;
WIFIConnectData connWifiData;


///////////////////////////////////////////////////////////////////////////////////////////////////////
wifiRecycAdapter::wifiRecycAdapter(ViewGroup *parent,RecyclerView *recyclerView):mParentView(parent),mRecyclerView(recyclerView){
    mWifi = sWifiData;
    mIsHeaderView = false;
    mConnectViewClickCallback = nullptr;
    if(g_appData.netSwitch) WIFIMgr::ins()->scanWifi();
}


RecyclerView::ViewHolder* wifiRecycAdapter::onCreateViewHolder(ViewGroup* parent, int viewType){
    ViewGroup *convertView;
    if(viewType == VIEW_TYPE_HEADER){
        convertView = (ViewGroup *)LayoutInflater::from(parent->getContext())->inflate("@layout/network_now", nullptr);
    }else{
        convertView = (ViewGroup *)LayoutInflater::from(parent->getContext())->inflate("@layout/network_list", nullptr);
    }
    return new wifiRecycAdapter::ViewHolder(convertView);
}
void wifiRecycAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position){
    // 设置背景图
    ViewGroup *viewGroup = ((wifiRecycAdapter::ViewHolder&)holder).viewGroup;
    LOGV("position = %d  viewGroup = %p",position,viewGroup);
    // viewGroup->setId(30000+position);
    // LOGE("viewGroup->getId() = %d",viewGroup->getId());
    if(mIsHeaderView && position == 0){
        ImageView* icon = (ImageView*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_icon);
        ProgressBar *connecting = (ProgressBar*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_connecting);
        TextView *connText = (TextView*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_conn_status);
        TextView* wifiName = (TextView*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_name);

        if(WIFIMgr::ins()->connStatus() == WIFI_CONNECTING){
            icon->setVisibility(View::INVISIBLE);
            connecting->setVisibility(View::VISIBLE);
            connecting->setIndeterminate(true);

            wifiName->setText(connWifiData.name);
            connText->setText("连接中");
        }else{
            connecting->setIndeterminate(false);
            connecting->setVisibility(View::INVISIBLE);
            icon->setVisibility(View::VISIBLE);
            // icon->setActivated(true);
            int level = g_appData.netStatus - PRO_STATE_NET_NONE;
            icon->setImageLevel(level<=0?1:(level>4?4:level));
            LOGI("network level = %d",level<=0?5:(level>4?4:level));

            wifiName->setText(g_objConf->getWifiName());
            connText->setText("已连接");
        }
        viewGroup->setOnTouchListener([this](View&v, MotionEvent&event){
            if(mConnectViewClickCallback) mConnectViewClickCallback(event.getAction());
            switch (event.getAction()) {
                case MotionEvent::ACTION_DOWN:
                    v.setPressed(true);
                    return true;
                case MotionEvent::ACTION_UP:
                case MotionEvent::ACTION_CANCEL:
                    v.setPressed(false);
                    return true;
            }
            return true;
        });
    }else{
        WIFIAdapterData pdat;
        if(mIsHeaderView)   pdat = mWifi.at(position-1);
        else                pdat = mWifi.at(position);
        TextView *wifi_name = (TextView *)viewGroup->findViewById(kaidu_qj2::R::id::network_list_name);
        wifi_name->setText(pdat.name); // 网络名

        // 信号图标
        ImageView* icon = (ImageView*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_icon);
        icon->setImageLevel(pdat.level<=0?5:(pdat.level>4?4:pdat.level));

        // 密码锁
        icon = (ImageView*)viewGroup->findViewById(kaidu_qj2::R::id::wifi_lock);
        icon->setVisibility(pdat.locked? View::VISIBLE: View::GONE);

        viewGroup->setOnClickListener([this,pdat](View& v) { 
            LOGI("pdat.locked = %d pdat.name = %s",pdat.locked,pdat.name.c_str()); 
            KInput* rename = new KInput(mParentView,"", "请输入 "+pdat.name+" 密码:",0,-1,
                [this,pdat](std::string inputData){
                    connWifiData.locked = pdat.locked;
                    connWifiData.name = pdat.name;
                    connWifiData.key = inputData;
                    for(auto it = sWifiData.begin(); it != sWifiData.end(); it++){
                        if(it->name == pdat.name){
                            g_appData.netStatus = it->level + PRO_STATE_NET_NONE;
                            sWifiData.erase(it);
                            break;
                        }
                    }
                    WIFIMgr::ins()->delayConnWifi(connWifiData,1);
                });
        });
    }
}
int wifiRecycAdapter::getItemCount(){
    LOGV("mWifi.size() = %d   mIsHeaderView = %d",mWifi.size(),mIsHeaderView);
    if(mIsHeaderView)   return mWifi.size()+1;
    else                return mWifi.size();
}

int wifiRecycAdapter::getItemViewType(int position){
    return (position == 0 && mIsHeaderView) ? VIEW_TYPE_HEADER : VIEW_TYPE_ITEM;
}

void wifiRecycAdapter::notifyData(){
    LOGE("wifiRecycAdapter::notifyData");
    mWifi = sWifiData;
    notifyDataSetChanged();
}

void wifiRecycAdapter::setHeaderView(bool isHeaderView){
    LOGE("isHeaderView = %d",isHeaderView);
    mIsHeaderView = isHeaderView;
}

void wifiRecycAdapter::setConnectViewClickCallback(connClickCallback callback){
    mConnectViewClickCallback = callback;
}

// void wifiRecycAdapter::onItemRangeChanged(int positionStart, int itemCount, Object* payload){
//     LOGE("positionStart = %d   itemCount = %d",positionStart,itemCount);
// }

// void wifiRecycAdapter::onViewRecycled(RecyclerView::ViewHolder& holder){
//     LOGE("holder.itemView = %p",holder.itemView);
// }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

int WIFIScanThread::onTask(void* data){
    std::vector<WIFIAdapterData> *dat = (std::vector<WIFIAdapterData> *)data;
    dat->clear();
    std::list<WifiSta::WIFI_ITEM_S> wifiList;
    WifiSta::ins()->scan(wifiList);
    std::string wifiName, wifiPasswd;
    g_objConf->getWifiInfo(wifiName, wifiPasswd); 

    for (WifiSta::WIFI_ITEM_S &item : wifiList) {
        if(item.signal <=0 ) continue;
        WIFIAdapterData wf_data;
        wf_data.locked      = item.isKey;
        wf_data.level       = item.signal;
        wf_data.conn_status = (wifiName == item.ssid) ? WIFI_CONNECTED : WIFI_DISCONNECTED;
        wf_data.name        = item.ssid;
        wf_data.quality     = item.quality;
        wf_data.signalLevel = item.signalLevel;
        if(((wf_data.name == wifiName) || (wf_data.name == connWifiData.name ))&& ((!g_appData.netOk && WIFIMgr::ins()->connStatus() == WIFI_CONNECTING) || g_appData.netOk )){
            g_appData.netStatus = PRO_STATE_NET_NONE + wf_data.level;
            LOGI_IF(wifiName == wf_data.name," /*   g_appData.netStatus  */ ");
        } else if((wf_data.name == wifiName) && (WIFIMgr::ins()->connStatus() == WIFI_DISCONNECTED) && !g_appData.netOk){
            connWifiData.name = wifiName;
            connWifiData.key = wifiPasswd;
            g_appData.netStatus = PRO_STATE_NET_NONE + wf_data.level;
            LOGE("/*  connWifiData.name = %s  connWifiData.key = %s  */ ",connWifiData.name.c_str(),connWifiData.key.c_str());
            WIFIMgr::ins()->delayConnWifi(connWifiData,1000);
        }else{
            dat->push_back(wf_data);
            LOGI_IF(wifiName == wf_data.name," /*  dat->push_back(wf_data)  */");
        }
            
    }
    return 0;
}

void WIFIScanThread::onMain(void *data) {
    static int count = 0;
    if(g_appData.netSwitch){
        std::vector<WIFIAdapterData> wifiScanList = *(std::vector<WIFIAdapterData> *)data;
        if((wifiScanList.size() == 0) && (WIFIMgr::ins()->connStatus() == WIFI_CONNECTED)){
            WIFIMgr::ins()->scanEnd();
            Activity_header::ins()->changeState((State)g_appData.netStatus);
            if((Activity_header::ins()->getActivityPageType() == ACTIVITY_SETUP) && (count < 5)){
                count++;
                WIFIMgr::ins()->scanWifi();
            }else{
                count = 0;
            }
            return;
        }
        sWifiData = wifiScanList;
        g_objWindMgr->delayAppCallback(CS_WIFI_ADAPTER_NOTIFI);
        Activity_header::ins()->changeState((State)g_appData.netStatus);
        count = 0;
        LOGE("scan wifi complet  sWifiData.size() = %d",sWifiData.size());
    }else{
        sWifiData.clear();
        LOGE("scan wifi fail!");
    }
    WIFIMgr::ins()->scanEnd();
}

int WIFIConnectThread::onTask(void* data){
    WIFIConnectData *dat = (WIFIConnectData *)data; 
    WifiSta::ins()->connect(dat->name,dat->key);
    return 0;
}

void WIFIConnectThread::onMain(void *data) {
    WIFIConnectData *dat = (WIFIConnectData *)data;
    connWifiData = {};
    if(g_appData.netSwitch){
        LOGE("connect wifi complet WifiSta::ins()->get_status() = %d",WifiSta::ins()->get_status());
        if(WifiSta::ins()->get_status() == WifiSta::E_CONNECT_SUCCESS){
            g_appData.netOk = true;
            g_objConf->setWifiInfo(dat->name,dat->key);
            for(auto it = sWifiData.begin(); it != sWifiData.end(); it++){
                if(it->name == dat->name){
                    g_appData.netStatus = it->level + PRO_STATE_NET_NONE;
                    sWifiData.erase(it);
                    break;
                }
            }
            WIFIMgr::ins()->setConnStatus(WIFI_CONNECTED);
            g_objWindMgr->AppCallback(CS_WIFI_CONNECT); 
            g_objWindMgr->AppCallback(CS_NETWORK_CHANGE);   
            g_appData.nextUpdateTime = SystemClock::uptimeMillis() + 3*1000;
        }else{
            g_appData.netStatus = PRO_STATE_NET_NONE;
            WIFIMgr::ins()->setConnStatus(WIFI_DISCONNECTED);
            WIFIMgr::ins()->scanWifi();
        }
        
#ifdef TUYA_OS
        if(g_appData.netOk){
            std::string authUuid;
            std::string authKey;
            g_objConf->getTuyaAuthCode(authUuid,authKey);
            if(authUuid == "none" && authKey == "none" || authUuid.empty() && authKey.empty()){
                g_objConf->getFromServerAuth();
                new Popwindow_tip_hint(nullptr,INPUT_FUNC_GET_AUTH);
            }else{
                g_tuyaOsMgr->init();
            }  
        }
#endif 
    }else{
        LOGE("g_appData.netSwitch is close !");
        g_appData.netStatus = PRO_STATE_NET_NONE;
        WifiSta::ins()->disconnect();
    }
    Activity_header::ins()->changeState((State)g_appData.netStatus);
}


WIFIMgr::WIFIMgr() {
}

void WIFIMgr::init(){
    mIsStartScan = false;
    mIsConnStatus = WIFI_DISCONNECTED;
    // HV_POPEN("ifconfig wlan0 up");
}
WIFIMgr::~WIFIMgr() {

}

void WIFIMgr::handleMessage(Message& message){
    if(message.what == MSG_DELAY_CONN){
        if(g_appData.netSwitch){
            mIsConnStatus = WIFI_CONNECTING;
            WifiSta::ins()->disconnect();
            Activity_header::ins()->changeState(PRO_STATE_NET_NONE);
            g_objWindMgr->delayAppCallback(CS_WIFI_CONNECTING);
            ThreadPool::ins()->add(new WIFIConnectThread, &mConnData,true);
            g_objWindMgr->AppCallback(CS_WIFI_CONNECT);
        }
    }else if(message.what == MSG_DELAY_DISCONN){
        if(mIsConnStatus == WIFI_CONNECTING && WifiSta::ins()->get_status() == WifiSta::E_STA_CONENCTING) disConnWifi();
    }
}

void WIFIMgr::scanWifi(){
    if(g_appData.netSwitch && !mIsStartScan){
        // system("ifconfig wlan0 up");
        mIsStartScan = true;
        ThreadPool::ins()->add(new WIFIScanThread, &sWifiScanData,true);
    }
}
void WIFIMgr::delayConnWifi(WIFIConnectData connWifiData,int time){
    if(g_appData.netSwitch){
        mConnData = connWifiData;
        Message mesg,mesg1;
        mesg.what = MSG_DELAY_CONN;
        mesg1.what = MSG_DELAY_DISCONN;

        Looper::getMainLooper()->removeMessages(this);
        Looper::getMainLooper()->sendMessageDelayed(time+30*1000,this,mesg1);
        Looper::getMainLooper()->sendMessageDelayed(time,this,mesg);
    }
}

void WIFIMgr::disConnWifi(){
    if(mIsConnStatus != WIFI_CONNECTING){ g_objConf->setWifiInfo("",""); }
    connWifiData = {};
    g_appData.netOk = false;
    g_appData.netStatus = PRO_STATE_NET_NONE;
    mIsConnStatus = WIFI_DISCONNECTED;
    WifiSta::ins()->disconnect();
    scanWifi();
    // if(mIsConnStatus == WIFI_CONNECTED){ 
        g_objWindMgr->AppCallback(CS_WIFI_CONNECT);
    // }
}

void WIFIMgr::scanEnd(){
    mIsStartScan = false;
}

void WIFIMgr::setConnStatus(int status){
    mIsConnStatus = status;
}
int  WIFIMgr::connStatus(){
    return mIsConnStatus;
}

void WIFIMgr::setWifiEnable(bool isEnable){
    if(isEnable){
        WifiSta::ins()->enableWifi();
    }else{
        WifiSta::ins()->disEnableWifi();
        WifiSta::ins()->disconnect();
        mIsConnStatus = WIFI_DISCONNECTED;
    }
}