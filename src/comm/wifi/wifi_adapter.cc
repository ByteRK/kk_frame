

#include "wifi_adapter.h"
#include "config_mgr.h"
#include "global_data.h"
#include "string_utils.h"
#include "wifi_set.h"
#include "base.h"
#include "R.h"
#include <core/app.h>
#include <widget/textview.h>

#include <hv_net.h>
#include <hv_icmp.h>

#define PING_FAIL_COUNT 3 /* 连续多次ping失败判定网络断开 */

void WIFIAdapter::autoCheck() {
    if (WIFIAdapter::instance()->mNetCheckTaskId > 0) return;
    WIFIAdapter::instance()->mNetCheckTaskId = ThreadPool::ins()->add(WIFIAdapter::instance(), 0);
}

int WIFIAdapter::speedLevel(int ms) {
    int netSignalLevel;
    if (ms < 300) {
        netSignalLevel = 4;
    } else if (ms < 400) {
        netSignalLevel = 3;
    } else if (ms < 500) {
        netSignalLevel = 2;
    } else {
        netSignalLevel = 1;
    }
    return netSignalLevel;
}

int WIFIAdapter::signalLevel(int val) {
    /* 0 rssi<=-100
       1 (-100, -88]
       2 (-88, -77]
       3 (-77, -55]
       4 rssi>-55 */
    if (val <= -100) {
        return 0;
    } else if (val <= -88) {
        return 1;
    } else if (val <= -77) {
        return 2;
    } else if (val <= -55) {
        return 3;
    } else {
        return 4;
    }
}

WIFIAdapter::WIFIAdapter() {
    mWifiTaskID = 0;
    mInterface = 0;
    mConnecting = false;
    mLoadComplete = false;
    mLastScanTime = 0;
    mNetCheckTaskId = 0;
    mPingFailCount = 0;
    mPingTimems = 0;
    mNetChange = 0;
}

void WIFIAdapter::setParent(Interface* parent) {
    mInterface = parent;

    if (mWifiTaskID == 0 && g_config->getWifi()) {
        mLastScanTime = SystemClock::uptimeMillis();
        mWifiTaskID = ThreadPool::ins()->add(this, 0);
    }
}

void WIFIAdapter::cancel() {
    mLoadComplete = false;
    mConnecting = false;
    mInterface = 0;

    if (mWifiTaskID > 0) {
        ThreadPool::ins()->del(mWifiTaskID);
        mWifiTaskID = 0;
    }
}

void WIFIAdapter::start() {
    if (!g_config->getWifi()) return;
    if (mWifiTaskID > 0) {
        LOGV("task not complete. taskId=%d", mWifiTaskID);
        return;
    }

    mConnecting = false;
    mLastScanTime = SystemClock::uptimeMillis();
    mWifiTaskID = ThreadPool::ins()->add(this, 0);
}

void WIFIAdapter::onTick() {
    if (!g_config->getWifi()) return;
    if (mConnecting) return;
    int64_t nowTick = SystemClock::uptimeMillis();
    if (nowTick - mLastScanTime < 3000) return;
    start();
}

void WIFIAdapter::stop() {
    if (mWifiTaskID > 0) {
        ThreadPool::ins()->del(mWifiTaskID);
        mWifiTaskID = 0;
    }

    mConnecting = true;
}

void WIFIAdapter::connecting(WIFIAdapterData* pdat) {
    mConnecting = true;

    for (WIFIAdapterData& wad : mShowWIFIData) {
        wad.conn_status = WIFI_DISCONNECTED;
    }
    pdat->conn_status = WIFI_CONNECTING;

    notifyDataSetChanged();
}

void WIFIAdapter::regNetChange(INetChange* inetchange) {
    mNetChange = inetchange;
}

int WIFIAdapter::getCount() const {
    if (!mLoadComplete || !g_config->getWifi()) return 0;
    LOGV("count=%d", mShowWIFIData.size());
    return mShowWIFIData.size();
}

void* WIFIAdapter::getItem(int position) const {
    if (position >= 0 && position < mShowWIFIData.size()) {
        return (void*)&mShowWIFIData.at(position);
    }
    LOGE("position out of data count!!! position=%d count=%d", position, mShowWIFIData.size());
    return 0;
}

View* WIFIAdapter::getView(int position, View* convertView, ViewGroup* parent) {
    LOGV("position=%d count=%d", position, mShowWIFIData.size());

    ViewGroup* vg = __dc(ViewGroup, convertView);
    if (!vg) {
        vg = mInterface->loadLayout("@layout/wifi_item");
    }

    WIFIAdapterData* pdat = (WIFIAdapterData*)getItem(position);

    // TextView* wifi_name = PBase::get<TextView>(vg, AppRid::name);
    // wifi_name->setText(pdat->name);

    // if (pdat->conn_status == WIFI_CONNECTED) {
    //     wifi_name->setTextColor(App::getInstance().getColor("@color/light_text"));
    // } else {
    //     wifi_name->setTextColor(App::getInstance().getColor("@color/normal_text"));
    // }

    // Drawable* leftDrawable = nullptr;
    // Drawable* rightDrawable = nullptr;
    // TextView* info = PBase::get<TextView>(vg, AppRid::info);
    // if (pdat->locked) {
    //     leftDrawable = App::getInstance().getDrawable("@mipmap/wifi_lock");
    // }

    // rightDrawable = App::getInstance().getDrawable(StringUtils::format("@mipmap/wifi_s%d", pdat->level));

    // info->setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, rightDrawable, 0);

    vg->setOnClickListener([this, pdat](View& v) { mInterface->onClickItem(__dc(ViewGroup, &v), pdat); });

    return vg;
}

int WIFIAdapter::onTask(int id, void* data) {
    if (id == mNetCheckTaskId) {
        char ip_addr[66] = { 0 };
        mPingTimems = 444;
#if defined(PRODUCT_X64)
        if (HV_NET_CheckNetStatus(&mPingTimems) > 0)
#else
        /* 公司网络连接受限，增加dns解析判定 */
        if (ping_host_ip("www.baidu.com", 1, &mPingTimems) == 0 ||
            host_to_ip("www.baidu.com", ip_addr, sizeof(ip_addr)))
#endif
        {
            mPingFailCount = 0;
        } else {
            mPingFailCount++;
            if (g_config->getWifi()) {
                LOGW_IF(mPingFailCount > 1, "check net status fail. count=%d", mPingFailCount);
                if (mPingFailCount > 1 && !g_data->mNetOK) {
                    SetWifi::autoConnect();
                }
            }
        }
        return 0;
    }

    std::list<WifiSta::WIFI_ITEM_S> wifiList;
    WifiSta::ins()->scan(wifiList);

    std::string wifiName, wifiPasswd;
    if (g_data->mNetOK) {
        wifiName = g_config->getWifiSSID(),
            wifiPasswd = g_config->getWifiPassword();
    }

    mDataMutex.lock();
    mThreadWIFIData.clear();
    for (WifiSta::WIFI_ITEM_S& item : wifiList) {
        WIFIAdapterData wf_data;
        wf_data.locked = item.encrypt;
        wf_data.level = item.signal;
        wf_data.conn_status = (wifiName == item.ssid) ? WIFI_CONNECTED : WIFI_DISCONNECTED;
        wf_data.name = item.ssid;
        mThreadWIFIData.push_back(wf_data);
    }
    mDataMutex.unlock();

    return 0;
}

void WIFIAdapter::onMain(int id, void* data) {
    if (id == mNetCheckTaskId) {
        if (mPingFailCount >= PING_FAIL_COUNT && g_data->mNetOK) {
            LOGE("net disconnected!!!");
            if (mNetChange) mNetChange->onNetChange(INetChange::NET_ERR);
        } else if (!g_data->mNetOK && mPingFailCount == 0) {
            LOGI("net connected!!!");
            if (mNetChange) mNetChange->onNetChange(INetChange::NET_OK);
        }
        if (g_data->mNetOK && speedLevel(mPingTimems) != speedLevel(1e6)) {
            LOGI("ping %dms", mPingTimems);
            if (mNetChange) mNetChange->onSignalChange(mPingTimems);
        }
        mNetCheckTaskId = 0;
        return;
    }

    mWifiTaskID = 0;
    if (mInterface && !mConnecting) {
        mDataMutex.lock();
        if (mThreadWIFIData.empty()) {
            LOGE("load wifi empty!!!");
            mShowWIFIData.clear();
        } else {
            mShowWIFIData.swap(mThreadWIFIData);
        }
        mDataMutex.unlock();
        mInterface->onData(mShowWIFIData);
        mLoadComplete = true;
        notifyDataSetChanged();
    }
}
