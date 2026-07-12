/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-28 11:40:38
 * @LastEditTime: 2026-06-30 14:29:40
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
    mInterfaceLifetime = parent ? parent->mLifetime : std::weak_ptr<int>();
    if (!getParent()) return;
    // 首次绑定时获取快照；重复绑定直接复用缓存，避免无效加锁。
    if (!mSourceApListInitialized) refreshApList(true);
    dispatchApListChanged();
}

void B_WifiAdapter::setConnectedDisplay(CONNECTED_DISPLAY type) {
    if (mConnectedItemDisplay == type) return;
    mConnectedItemDisplay = type;
    // 显示策略变化只需重新处理原始缓存；无缓存且已绑定时才读取 WifiMgr。
    if (mSourceApListInitialized) processApList();
    else if (getParent()) refreshApList(true);
    dispatchApListChanged();
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

    clearApList(true);
    Interface* parent = getParent();
    if (parent) parent->flushEnd();
}

void B_WifiAdapter::onScanResult() {
    refreshApList(true);
    dispatchApListChanged();
}

void B_WifiAdapter::refreshApList(bool reloadSource) {
    if (reloadSource) {
        mSourceApInfoList.clear();
        if (g_wifi->getState() != WifiHal::State::Off) {
            g_wifi->getAps(mSourceApInfoList);
        }
        mSourceApListInitialized = true;
    }
    processApList();
}

void B_WifiAdapter::processApList() {
    mApInfoList = mSourceApInfoList;
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
}

void B_WifiAdapter::clearApList(bool sourceInitialized) {
    mSourceApInfoList.clear();
    mApInfoList.clear();
    mSourceApListInitialized = sourceInitialized;
}

void B_WifiAdapter::dispatchApListChanged() {
    Interface* parent = getParent();
    if (!parent) return;
    parent->flushEnd();
    notifyApListChanged();
}

B_WifiAdapter::Interface* B_WifiAdapter::getParent() {
    if (!mInterface || mInterfaceLifetime.expired()) {
        mInterface = nullptr;
        mInterfaceLifetime.reset();
        return nullptr;
    }
    return mInterface;
}

WifiAdapter::WifiAdapter() {
    g_wifi->addListener(this);
}

WifiAdapter::~WifiAdapter() {
    g_wifi->removeListener(this);
}

void WifiAdapter::clear() {
    clearApList(false);
    if (getParent()) notifyDataSetChanged();
}

void WifiAdapter::onStateChanged() {
    const size_t oldCount = mApInfoList.size();
    B_WifiAdapter::onStateChanged();
    if (getParent() && mApInfoList.size() != oldCount) notifyDataSetChanged();
}

void WifiAdapter::notifyApListChanged() {
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
    Interface* interface = getParent();
    if (!interface || !apInfo) {
        LOGE("getView failed. interface=%p apInfo=%p",
            static_cast<void*>(interface), static_cast<void*>(apInfo));
        return convertView;
    }

    if (!convertView) convertView = interface->loadItemLayout(apInfo->connected);
    if (!convertView) {
        LOGE("loadItemLayout failed. position=%d", position);
        return nullptr;
    }

    interface->setItemLayout(position, convertView, apInfo);
    const std::string bssid = apInfo->bssid;
    const std::string ssid = apInfo->ssid;
    convertView->setOnClickListener([this, bssid, ssid](View& v) {
        Interface* interface = getParent();
        if (!interface) return;

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

        interface->onClickItem(&v, &*iterator);
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
    clearApList(false);
    if (getParent()) notifyDataSetChanged();
}

void WifiRecycleAdapter::onStateChanged() {
    const size_t oldCount = mApInfoList.size();
    B_WifiAdapter::onStateChanged();
    if (mApInfoList.size() != oldCount && getParent()) notifyDataSetChanged();
}

void WifiRecycleAdapter::notifyApListChanged() {
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
    Interface* interface = getParent();
    if (!interface) {
        LOGE("onCreateViewHolder failed: interface is null");
        return nullptr;
    }

    View* itemView = interface->loadItemLayout(viewType);
    if (!itemView) {
        LOGE("loadItemLayout failed. viewType=%d", viewType);
        return nullptr;
    }

    return new RecyclerView::ViewHolder(itemView);
}

void WifiRecycleAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    WifiHal::ApInfo* apInfo = getItem(position);
    View* itemView = holder.itemView;
    Interface* interface = getParent();
    if (!interface || !apInfo || !itemView) {
        LOGE("onBindViewHolder failed. interface=%p apInfo=%p itemView=%p",
            static_cast<void*>(interface), static_cast<void*>(apInfo), static_cast<void*>(itemView));
        return;
    }

    interface->setItemLayout(position, itemView, apInfo);

    RecyclerView::ViewHolder* h_ptr = &holder;
    itemView->setOnClickListener([this, h_ptr](View& v) {
        onClickItem(&v, h_ptr);
        });
}

void WifiRecycleAdapter::onClickItem(View* v, RecyclerView::ViewHolder* holder) {
    Interface* interface = getParent();
    if (!interface || !v || !holder) return;

    const int position = holder->getAbsoluteAdapterPosition();
    if (position == RecyclerView::NO_POSITION) return;

    WifiHal::ApInfo* apInfo = getItem(position);
    if (apInfo) interface->onClickItem(v, apInfo);
}

#endif // !ENABLED(WIFI)
