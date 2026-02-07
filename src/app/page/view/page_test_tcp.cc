/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 17:16:25
 * @LastEditTime: 2026-02-07 17:07:45
 * @FilePath: /kk_frame/src/app/page/view/page_test_tcp.cc
 * @Description: TCP 测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_test_tcp.h"
#include <widget/textview.h>


#define LISTEN_PORT 23333

void EchoServerLogic::onConnected(int id) {
    mLastClientId = id;
    LOGI("[Server] client %d connected", id);
}

void EchoServerLogic::onDisconnected(int id) {
    LOGI("[Server] client %d disconnected", id);
}

void EchoServerLogic::onRecv(const uint8_t* data, size_t len, int id) {
    mLastClientId = id;
    LOGE("[Server] recv from %d: %.*s", id, (int)len, data);
}

void EchoClientLogic::onConnected(int id) {
    LOGI("[Client] connected to server");
}

void EchoClientLogic::onDisconnected(int id) {
    LOGI("[Client] disconnected");
}

void EchoClientLogic::onRecv(const uint8_t* data, size_t len, int id) {
    LOGE("[Client] recv: %.*s", (int)len, data);
}


PAGE_REGISTER(PAGE_TEST_TCP, TestTcpPage);

TestTcpPage::TestTcpPage() :PageBase("@layout/page_demo") {
    initUI();

    // ---------- Server ----------
    mServerLogic = new EchoServerLogic();
    mTcpServer = new TcpServer(LISTEN_PORT);
    mTcpServer->setHandler(mServerLogic);
    mTcpServer->init();
    mTcpServer->start();
    LOGE("[Server] server start");

    // ---------- Client ----------
    mClientLogic = new EchoClientLogic();
    mTcpClient = new TcpClient("127.0.0.1", LISTEN_PORT);
    mTcpClient->setHandler(mClientLogic);
    mTcpClient->init();
    mTcpClient->start();
    LOGE("[Client] client start");
}

TestTcpPage::~TestTcpPage() {
    if (mTcpClient) {
        mTcpClient->stop();
        __delete(mTcpClient);
    }
    if (mTcpServer) {
        mTcpServer->stop();
        __delete(mTcpServer);
    }
    if (mServerLogic) {
        __delete(mServerLogic);
    }
    if (mClientLogic) {
        __delete(mClientLogic);
    }
}

int8_t TestTcpPage::getType() const {
    return PAGE_TEST_TCP;
}

void TestTcpPage::setView() {
    TextView* tv = get<TextView>(AppRid::hello);
    if (tv) {
        tv->setText("TCP 测试[" + std::to_string(LISTEN_PORT) + "]\n\n点击屏幕发送消息\n点击左发送给右   点击右发送给左\n\n\n|\n客户端 | 服务端\n|\n");
        tv->setGravity(Gravity::CENTER);
        tv->setTextSize(20);
    }

    mRootView->setOnTouchListener(std::bind(&TestTcpPage::onTouchListen, this, std::placeholders::_1, std::placeholders::_2));
}

void TestTcpPage::onTick() {
}

bool TestTcpPage::onTouchListen(View& v, MotionEvent& e) {
    static int s_ScreenWidth = v.getWidth();
    if (e.getAction() != MotionEvent::ACTION_DOWN) return true;

    if (e.getX() < s_ScreenWidth / 2) {
        const char* msg = "Hello";
        LOGW("[Client] send: %s", msg);
        mTcpClient->send((const uint8_t*)msg, 5);
    } else {
        const char* msg = "World";
        LOGW("[Server] send: %s", msg);
        mTcpServer->send((const uint8_t*)msg, 5, mServerLogic->mLastClientId);
    }
    return true;
}
