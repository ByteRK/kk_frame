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
}

void B_WifiAdapter::setConnectedDisplay(CONNECTED_DISPLAY type) {
    mConnectedItemDisplay = type;
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
}

void B_WifiAdapter::onScanResult() {
    g_wifi->getAps(mApInfoList);
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

void WifiAdapter::onScanResult() {
    B_WifiAdapter::onScanResult();
    notifyDataSetChanged();
}

int WifiAdapter::getCount() const {
    return mApInfoList.size();
}

void* WifiAdapter::getItem(int position) const {
    if (position >= 0 && position < mApInfoList.size()) {
        return (void*)&mApInfoList.at(position);
    }
    LOGE("position out of data count!!! position=%d count=%d", position, mApInfoList.size());
    return nullptr;
}

View* WifiAdapter::getView(int position, View* convertView, ViewGroup* parent) {
    LOGV("position=%d count=%d", position, mApInfoList.size());
    WifiHal::ApInfo* apInfo = (WifiHal::ApInfo*)getItem(position);

    if (!convertView) convertView = mInterface->loadItemLayout(apInfo->connected);

    mInterface->setItemLayout(position, convertView, apInfo);
    convertView->setOnClickListener([this, apInfo](View& v) { mInterface->onClickItem(__dc(ViewGroup, &v), apInfo); });

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

void WifiRecycleAdapter::onScanResult() {
    B_WifiAdapter::onScanResult();
    notifyDataSetChanged();
}

WifiHal::ApInfo* WifiRecycleAdapter::getItem(int position) {
    if (position >= 0 && position < mApInfoList.size())
        return &mApInfoList.at(position);
    LOGE("position out of data count!!! position=%d count=%d", position, mApInfoList.size());
    return nullptr;
}

int WifiRecycleAdapter::getItemCount() {
    return mApInfoList.size();
}

int WifiRecycleAdapter::getItemViewType(int position) {
    WifiHal::ApInfo* apInfo = getItem(position);
    return apInfo->connected;
}

RecyclerView::ViewHolder* WifiRecycleAdapter::onCreateViewHolder(ViewGroup* parent, int viewType) {
    return new RecyclerView::ViewHolder(mInterface->loadItemLayout(viewType));
}

void WifiRecycleAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    WifiHal::ApInfo* apInfo = getItem(position);
    ViewGroup* vg = __dc(ViewGroup, holder.itemView);
    mInterface->setItemLayout(position, vg, apInfo);

    RecyclerView::ViewHolder* h_ptr = &holder;
    vg->setOnClickListener([this, h_ptr](View& v) {
        onClickItem(&v, h_ptr);
    });
}

void WifiRecycleAdapter::onClickItem(View* v, RecyclerView::ViewHolder* holder) {
    int position = holder->getAbsoluteAdapterPosition();
    mInterface->onClickItem(v, getItem(position));
}

#endif // !ENABLED(WIFI)
