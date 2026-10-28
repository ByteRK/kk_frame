/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_auth.cc
 * @Description: 授权信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_auth.h"

#include "auth_mgr.h"
#include "config_mgr.h"
#include "string_utils.h"

FactoryAuth::FactoryAuth(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_auth") {
    setExitBtn(AppRid::exit);

    mAuthInfo = PBase::get<TextView>(mRootView, AppRid::auth_info);
    mAuthRequest = PBase::get<TextView>(mRootView, AppRid::auth_request);
    mAuthRequestInfo = PBase::get<TextView>(mRootView, AppRid::auth_request_info);

    PBase::click(mAuthRequest, [](View&) {
        g_auth->request();
    });

    g_msg->add(MSG_AUTH_CODE, this);
}

FactoryAuth::~FactoryAuth() {
    g_msg->removeAll(this);
}

void FactoryAuth::onShow() {
    onMessage(MSG_AUTH_CODE, 0, nullptr);
}

void FactoryAuth::onMessage(int msgType, int msgValue, void* msgPtr) {
    if (msgType != MSG_AUTH_CODE) return;

    mAuthRequestInfo->setText(
        g_auth->mac() + "\n" + g_auth->authURL()
    );

    if (g_config->isAuthSuccess()) {
        mAuthInfo->setText(
            StringUtils::format(
                "授权码信息\nPIP: %s | DID: %s",
                g_config->getProductId().c_str(),
                g_config->getDeviceId().c_str()
            )
        );
        mAuthRequest->setText("再次请求");
        mAuthRequest->setActivated(true);
    } else {
        mAuthInfo->setText("当前不存在授权码");
        mAuthRequest->setText("申请授权码");
        mAuthRequest->setActivated(false);
    }
}

FACTORY_ITEM_REGISTER(AppRid::to_auth, FactoryAuth);
