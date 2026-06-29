/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-28 11:40:38
 * @LastEditTime: 2026-02-28 17:15:22
 * @FilePath: /kk_frame/src/class/wifi_adapter.cc
 * @Description: WIFI适配器（UI用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wifi_adapter.h"

#if ENABLED(WIFI) || defined(__VSCODE__)

#include "config_mgr.h"
#include "global_data.h"
#include "string_utils.h"
#include "base.h"
#include <core/app.h>


void B_WifiAdapter::setParent(Interface* parent) {
    mInterface = parent;
    if (mInterface) onScanResult();
}

void B_WifiAdapter::setConnectedDisplay(CONNECTED_DISPLAY type) {
    if (mConnectedItemDisplay == type) return;
    mConnectedItemDisplay = type;
    onScanResult();
}

int B_WifiAdapter::speedLevel(int ms) {
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

int B_WifiAdapter::signalLevel(int val) {
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

void B_WifiAdapter::onStateChanged() {
    if (g_wifi->getState() != WifiHal::State::Off) return;

    mApInfoList.clear();
    if (mInterface) mInterface->flushEnd();
}

void B_WifiAdapter::onScanResult() {
    if (g_wifi->getState() == WifiHal::State::Off) mApInfoList.clear();
    else g_wifi->getAps(mApInfoList);
    if (mConnectedItemDisplay != DISPLAY_TYPE_KEEP) { // 非保持原列表
        auto iterator = std::find_if(mApInfoList.begin(), mApInfoList.end(), [](const WifiHal::ApInfo& ap) {
            return ap.connected;
        });
        if (iterator != mApInfoList.end()) {
            if (mConnectedItemDisplay == DISPLAY_TYPE_TOP) { // 置顶
                WifiHal::ApInfo connectedAp = *iterator;
                mApInfoList.erase(iterator);
                mApInfoList.insert(mApInfoList.begin(), connectedAp);
            } else { // 移除
                mApInfoList.erase(iterator);
            }
        }
    }
    if (mInterface)mInterface->flushEnd();
}

WifiAdapter::WifiAdapter() {
    g_wifi->addListener(this);
}

WifiAdapter::~WifiAdapter() {
    g_wifi->removeListener(this);
}

void WifiAdapter::clear() {
    mApInfoList.clear();
    notifyDataSetChanged();
}

void WifiAdapter::onStateChanged() {
    const size_t oldCount = mApInfoList.size();
    B_WifiAdapter::onStateChanged();
    if (mApInfoList.size() != oldCount) notifyDataSetChanged();
}

void WifiAdapter::onScanResult() {
    B_WifiAdapter::onScanResult();
    notifyDataSetChanged();
}

int WifiAdapter::getCount() const {
    return mApInfoList.size();
}

void* WifiAdapter::getItem(int position) const {
    if (position >= 0 && static_cast<size_t>(position) < mApInfoList.size()) {
        return const_cast<WifiHal::ApInfo*>(&mApInfoList.at(position));
    }
    LOGE("position out of data count!!! position=%d count=%zu", position, mApInfoList.size());
    return nullptr;
}

int WifiAdapter::getItemViewType(int position) const {
    const auto* apInfo = static_cast<const WifiHal::ApInfo*>(getItem(position));
    return apInfo && apInfo->connected ? 1 : 0;
}

int WifiAdapter::getViewTypeCount() const {
    return 2;
}

View* WifiAdapter::getView(int position, View* convertView, ViewGroup* parent) {
    LOGV("position=%d count=%zu", position, mApInfoList.size());
    auto* apInfo = static_cast<WifiHal::ApInfo*>(getItem(position));
    if (!mInterface || !apInfo) {
        LOGE("getView failed. interface=%p apInfo=%p",
            static_cast<void*>(mInterface), static_cast<void*>(apInfo));
        return convertView;
    }

    if (!convertView) convertView = mInterface->loadItemLayout(apInfo->connected);
    if (!convertView) {
        LOGE("loadItemLayout failed. position=%d", position);
        return nullptr;
    }

    mInterface->setItemLayout(position, convertView, apInfo);
    const std::string bssid = apInfo->bssid;
    const std::string ssid = apInfo->ssid;
    convertView->setOnClickListener([this, bssid, ssid](View& v) {
        if (!mInterface) return;

        auto iterator = std::find_if(mApInfoList.begin(), mApInfoList.end(),
            [&bssid, &ssid](const WifiHal::ApInfo& ap) {
                if (ap.ssid != ssid) return false;
#if WIFI_ADAPTER_CLICK_MATCH_BSSID
                return ap.bssid == bssid;
#else
                return true;
#endif
            });
        if (iterator == mApInfoList.end()) {
            LOGW("clicked AP no longer exists. bssid=%s ssid=%s", bssid.c_str(), ssid.c_str());
            return;
        }

        mInterface->onClickItem(&v, &*iterator);
    });

    return convertView;
}


WifiRecycleAdapter::WifiRecycleAdapter() {
    g_wifi->addListener(this);
}

WifiRecycleAdapter::~WifiRecycleAdapter() {
    g_wifi->removeListener(this);
}

void WifiRecycleAdapter::clear() {
    mApInfoList.clear();
    notifyDataSetChanged();
}

void WifiRecycleAdapter::onStateChanged() {
    const size_t oldCount = mApInfoList.size();
    B_WifiAdapter::onStateChanged();
    if (mApInfoList.size() != oldCount) notifyDataSetChanged();
}

void WifiRecycleAdapter::onScanResult() {
    B_WifiAdapter::onScanResult();
    notifyDataSetChanged();
}

WifiHal::ApInfo* WifiRecycleAdapter::getItem(int position) {
    if (position >= 0 && static_cast<size_t>(position) < mApInfoList.size())
        return &mApInfoList.at(position);
    LOGE("position out of data count!!! position=%d count=%zu", position, mApInfoList.size());
    return nullptr;
}

int WifiRecycleAdapter::getItemCount() {
    return mApInfoList.size();
}

int WifiRecycleAdapter::getItemViewType(int position) {
    WifiHal::ApInfo* apInfo = getItem(position);
    return apInfo && apInfo->connected ? 1 : 0;
}

RecyclerView::ViewHolder* WifiRecycleAdapter::onCreateViewHolder(ViewGroup* parent, int viewType) {
    if (!mInterface) {
        LOGE("onCreateViewHolder failed: interface is null");
        return nullptr;
    }

    View* itemView = mInterface->loadItemLayout(viewType);
    if (!itemView) {
        LOGE("loadItemLayout failed. viewType=%d", viewType);
        return nullptr;
    }

    return new RecyclerView::ViewHolder(itemView);
}

void WifiRecycleAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    WifiHal::ApInfo* apInfo = getItem(position);
    View* itemView = holder.itemView;
    if (!mInterface || !apInfo || !itemView) {
        LOGE("onBindViewHolder failed. interface=%p apInfo=%p itemView=%p",
            static_cast<void*>(mInterface), static_cast<void*>(apInfo), static_cast<void*>(itemView));
        return;
    }

    mInterface->setItemLayout(position, itemView, apInfo);

    RecyclerView::ViewHolder* h_ptr = &holder;
    itemView->setOnClickListener([this, h_ptr](View& v) {
        onClickItem(&v, h_ptr);
    });
}

void WifiRecycleAdapter::onClickItem(View* v, RecyclerView::ViewHolder* holder) {
    if (!mInterface || !v || !holder) return;

    const int position = holder->getAbsoluteAdapterPosition();
    if (position == RecyclerView::NO_POSITION) return;

    WifiHal::ApInfo* apInfo = getItem(position);
    if (apInfo) mInterface->onClickItem(v, apInfo);
}

#endif // !ENABLED(WIFI)
