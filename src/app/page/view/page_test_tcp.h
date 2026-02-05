/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 17:16:11
 * @LastEditTime: 2026-02-05 17:36:41
 * @FilePath: /kk_frame/src/app/page/view/page_test_tcp.h
 * @Description: TCP 测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PAGE_TEST_TCP_H__
#define __PAGE_TEST_TCP_H__

#include "page.h"

#include "tcp_client.h"
#include "tcp_server.h"

class EchoServerLogic : public ITcpServerHandler {
public:
    int mLastClientId = 0;
    EchoServerLogic() = default;
    void onClientConnected(int clientId) override;
    void onClientDisconnected(int clientId) override;
    void onRecv(int clientId, const uint8_t* data, size_t len) override;
};

class EchoClientLogic : public ITcpClientHandler {
public:
    EchoClientLogic() = default;
    void onConnected() override;
    void onDisconnected() override;
    void onRecv(const uint8_t* data, size_t len) override;
};


class TestTcpPage :public PageBase {
private:
    EchoClientLogic* mClientLogic;
    TcpClientTransport* mTcpClient;

    EchoServerLogic* mServerLogic;
    TcpServerTransport* mTcpServer;
public:
    TestTcpPage();
    ~TestTcpPage();
    int8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;

private:
    bool onTouchListen(View& v, MotionEvent& e);
};

#endif /* __PAGE_TEST_TCP_H__ */
