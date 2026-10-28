/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_auth.h
 * @Description: 授权信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_AUTH_H__
#define __FACTORY_AUTH_H__

#include "page_factory.h"
#include "message_mgr.h"

/// @brief 授权信息
class FactoryAuth : public PageFactory::FactoryItem,
    public MessageListener {
private:
    TextView* mAuthInfo{ nullptr };
    TextView* mAuthRequest{ nullptr };
    TextView* mAuthRequestInfo{ nullptr };

public:
    FactoryAuth(ViewGroup* container, PageFactory* factory);
    ~FactoryAuth() override;

protected:
    void onShow() override;
    void onMessage(int msgType, int msgValue = 0, void* msgPtr = nullptr) override;
};

#endif // __FACTORY_AUTH_H__
