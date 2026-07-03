/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-03 17:05:27
 * @LastEditTime: 2026-07-04 04:30:45
 * @FilePath: /kk_frame/src/app/page/view/page_factory_item.h
 * @Description: 产测页面子项
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PAGE_FACTORY_ITEM_H__
#define __PAGE_FACTORY_ITEM_H__

#include "page_factory.h"
#include "tick_mgr.h"
#include "wifi_mgr.h"
#include "message_mgr.h"

/// @brief 项目信息
class FactoryInfo : public PageFactory::FactoryItem {
public:
    FactoryInfo(View* view, PageFactory* factory);
private:
    void setInfo();
};

/// @brief 网络信息
class FactoryNetwork : public PageFactory::FactoryItem {
public:
    FactoryNetwork(View* view, PageFactory* factory);
private:
    void setInterface();
};

/// @brief 授权信息
class FactoryAuth : public PageFactory::FactoryItem,
    public MessageListener {
private:
    TextView* mAuthInfo{ nullptr };
    TextView* mAuthRequest{ nullptr };
    TextView* mAuthRequestInfo{ nullptr };
public:
    FactoryAuth(View* view, PageFactory* factory);
    ~FactoryAuth() override;
protected:
    void onShow() override;
    void onMessage(int msgType, int msgValue = 0, void* msgPtr = nullptr) override;
};

/// @brief 功能开关
class FactorySwitch : public PageFactory::FactoryItem {
public:
    FactorySwitch(View* view, PageFactory* factory);
private:
    void setSwitch();
};

/// @brief 触摸测试
class FactoryTouch : public PageFactory::FactoryItem {
public:
    FactoryTouch(View* view, PageFactory* factory);
protected:
    void onShow() override;
private:
    void setTouch();
};

/// @brief 显示测试
class FactoryColor : public PageFactory::FactoryItem {
public:
    FactoryColor(View* view, PageFactory* factory);
protected:
    void onShow() override;
private:
    void setColor();
};

/// @brief WIFI检测
class FactoryWifi : public PageFactory::FactoryItem,
    public WifiMgr::WiFiListener {
private:
    TextView* mWifiList{ nullptr };
    TextView* mWifiConnInfo{ nullptr };
    TextView* mWifiRefresh{ nullptr };
    TextView* mWifiSwitch{ nullptr };
public:
    FactoryWifi(View* view, PageFactory* factory);
    ~FactoryWifi() override;
protected:
    void onShow() override;
    void onStateChanged() override;
    void onScanResult() override;
private:
    void updateWifiInfo();
};

/// @brief 屏幕老化
class FactoryAging : public PageFactory::FactoryItem {
private:
    TickMgr::ITickVariable mTicker;
public:
    FactoryAging(View* view, PageFactory* factory);
protected:
    void onShow() override;
    void onHide() override;
private:
    void onTick(int64_t);
};


#endif // __PAGE_FACTORY_ITEM_H__
