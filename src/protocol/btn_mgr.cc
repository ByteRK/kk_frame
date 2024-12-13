#include "btn_mgr.h"
#include <core/app.h>

#include "config_mgr.h"

#include "manage.h"
#include "this_func.h"

#define TICK_TIME 50 // tick触发时间（毫秒）

enum {
    BTN_Q = 0b1,
    BTN_A = 0b10,
    BTN_Z = 0b100,
    BTN_W = 0b1000,
    BTN_S = 0b10000,
    BTN_E = 0b100000,
    BTN_D = 0b1000000,
    BTN_R = 0b10000000,
    BTN_F = 0b100000000,
};

enum {
    BTN_U = 0b1,
    BTN_H = 0b10,
    BTN_I = 0b100,
    BTN_J = 0b1000,
    BTN_O = 0b10000,
    BTN_K = 0b100000,
    BTN_P = 0b1000000,
    BTN_L = 0b10000000,
    BTN_M = 0b100000000,
};

static std::map<int, int> keyMapA = {
    {BTN_Q,KEY_Q},{BTN_A,KEY_A},{BTN_Z,KEY_Z},{BTN_W,KEY_W},{BTN_S,KEY_S},{BTN_E,KEY_E},{BTN_D,KEY_D},{BTN_R,KEY_R},{BTN_F,KEY_F},
};

static std::map<int, int> keyMapB = {
    {BTN_U,KEY_U},{BTN_H,KEY_H},{BTN_I,KEY_I},{BTN_J,KEY_J},{BTN_O,KEY_O},{BTN_K,KEY_K},{BTN_P,KEY_P},{BTN_L,KEY_L},{BTN_M,KEY_M},
};

//////////////////////////////////////////////////////////////////

BtnMgr::BtnMgr() {
    mPacket = new SDHWPacketBuffer();
    mI2CBTN = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;
    mLastStatusTime = 0;
    mLastWarnTime = 0;
    mFirstSend = true;

    mVersionL = 0;
    mVersionR = 0;

    LK_TEST = 0;

    mPacket->setType(BT_BTN, BT_BTN);
    CHandlerManager::ins()->addHandler(BT_BTN, this);
}

BtnMgr::~BtnMgr() {
    __del(mI2CBTN);
}

int BtnMgr::init() {

#ifndef CDROID_X64
    mI2CBTN = new I2CClient(mPacket, BT_BTN, 100);
    mI2CBTN->init();
#endif

    mSendLeft = false;
    memset(mBtnLight, BTN_CLOSE, ALL_BTN_COUNT);


    // 启动延迟一会后开始发包
    mNextEventTime = SystemClock::uptimeMillis() + TICK_TIME * 10;
    App::getInstance().addEventHandler(this);
    return 0;
}

std::string BtnMgr::getVersion() {
    return std::to_string(mVersionL) + "." + std::to_string(mVersionR);
}

int BtnMgr::checkEvents() {
    int64_t curr_tick = SystemClock::uptimeMillis();
    if (curr_tick >= mNextEventTime) {
        mNextEventTime = curr_tick + TICK_TIME;
        return 1;
    }
    return 0;
}

int BtnMgr::handleEvents() {
    int64_t now_tick = SystemClock::uptimeMillis();

    if (mI2CBTN) {
        mI2CBTN->onTick();
        if (mFirstSend) {
            mFirstSend = false;
            send2MCU();
        } else {
            send2MCU();
        }
    }

    return 1;
}

void BtnMgr::send2MCU() {
    BuffData* bd = mPacket->obtain(BT_BTN, 0);
    UI2MCU   snd(bd, BT_BTN);

    if (mSendLeft) {
        snd.setData(2, 0x0B);
        for (int n = 3, i = 9; n <= 11; i++, n++)
            snd.setData(n, mBtnLight[i]);
    } else {
        snd.setData(2, 0x0A);
        for (int n = 3, i = 0; n <= 11; i++, n++)
            snd.setData(n, mBtnLight[i]);
    }
    mSendLeft = !mSendLeft;

    snd.checkcode();    // 修改检验位

    // LOG(VERBOSE) << "send to btn. bytes=" << hexstr(bd->buf, bd->len);
    mI2CBTN->send(bd);
    mLastSendTime = SystemClock::uptimeMillis();
}

void BtnMgr::onCommDeal(IAck* ack) {
    LOG(VERBOSE) << "hex str=" << hexstr(ack->mBuf, ack->mDlen);
    int64_t now_tick = SystemClock::uptimeMillis();

    static ushort clickType = 0x0000, clickValue = 0;
    ushort type = ack->getData(2), value = ack->getData2(3, true);

    switch (type) {
    case 0x0A:
        mVersionL = ack->getData(7);
        break;
    case 0x0B:
        mVersionR = ack->getData(7);
    default:
        break;
    }

    dealLK(type, value);

    if (value == 0 && clickValue != 0 && clickType == type) {
        sendButton(clickType, clickValue, HW_EVENT_UP);
        clickType = 0x00;
        clickValue = 0;
    } else if (value != 0 && clickValue == 0) {
        sendButton(type, value, HW_EVENT_DOWN);
        clickType = type;
        clickValue = value;
        mDownTime = now_tick;
    } else if (clickValue != 0 && now_tick - mDownTime >= 700) {
        sendButton(clickType, clickValue, HW_EVENT_LONG);
    }
}

void BtnMgr::sendButton(uchar type, ushort key, uchar status) {
    LOG(VERBOSE) << "type" << type << "sendButton = " << key << " status = " << status;
    if (type == 0x0A) {
        auto it = keyMapA.find(key);
        if (it != keyMapA.end()) {
            LOGV("key = %d status = %d", it->second, status);
            g_windMgr->sendKey(it->second, status);
        }
    } else {
        auto it = keyMapB.find(key);
        if (it != keyMapB.end()) {
            LOGV("key = %d status = %d", it->second, status);
            g_windMgr->sendKey(it->second, status);
        }
    }
}

void BtnMgr::dealLK(uchar type, ushort value) {
    if (type == 0x0A) {
        if (value == BTN_F) LK_TEST++;else LK_TEST = 0;
    } else {
        if (value == BTN_H) LK_TEST++;else LK_TEST = 0;
    }
    LOGV("KEY %x|%d LK_TEST = %d", type, value, LK_TEST);
    if (LK_TEST > 60)g_windMgr->specialKey(0);
}

void BtnMgr::setLight(uchar* left, uchar* right) {
    memcpy(mBtnLight, left, 9);
    memcpy(mBtnLight + 9, right, 9);
}
