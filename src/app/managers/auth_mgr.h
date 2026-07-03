/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-04 00:39:12
 * @LastEditTime: 2026-07-04 05:06:03
 * @FilePath: /kk_frame/src/app/managers/auth_mgr.h
 * @Description: 授权码管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __APP_AUTH_H__
#define __APP_AUTH_H__

#include "http_mgr.h"
#include "singleton.h"

#define g_auth AuthMgr::instance()

class AuthMgr :public HttpManager::IEventListener,
    public Singleton<AuthMgr> {
    friend class Singleton<AuthMgr>;

private:
    HttpManager::RequestId mReqId{ 0 };

protected:
    AuthMgr();

public:
    bool        request();

    std::string mac();
    std::string authURL();

protected:
    void        onHttpEvent(const HttpManager::Event& event);
};

#endif // __APP_AUTH_H__
