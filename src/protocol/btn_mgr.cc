#include "btn_mgr.h"
#include <core/app.h>

#include "config_mgr.h"

#include "manage.h"
#include "this_func.h"

#define TICK_TIME 50 // tick触发时间（毫秒）

enum {
    BTN_Q = 0b1,
    BTN_A = 0b10,
    BTN_W = 0b100,
    BTN_S = 0b1000,
    BTN_E = 0b10000,
    BTN_R = 0b100000,
    BTN_F = 0b1000000,
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
};

static std::map<int, int> keyMap = {
    {BTN_Q,KEY_Q},{BTN_A,KEY_A},{BTN_W,KEY_W},{BTN_S,KEY_S},{BTN_E,KEY_E},{BTN_R,KEY_R},{BTN_F,KEY_F},
    {BTN_U * 10,KEY_U},{BTN_H * 10,KEY_H},{BTN_I * 10,KEY_I},{BTN_J * 10,KEY_J},{BTN_O * 10,KEY_O},{BTN_K * 10,KEY_K},{BTN_P * 10,KEY_P},{BTN_L * 10,KEY_L}
};

//////////////////////////////////////////////////////////////////

BtnMgr::BtnMgr() {
    mPacket = new SDHWPacketBuffer();
    mUartBTN = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;
    mLastStatusTime = 0;
    mLastWarnTime = 0;
    mFirstSend = true;

    mVersionL = 0;
    mVersionR = 0;

    mPacket->setType(BT_BTN, BT_BTN);
    CHandlerManager::ins()->addHandler(BT_BTN, this);
}

BtnMgr::~BtnMgr() {
    __del(mUartBTN);
}

int BtnMgr::init() {
    LOGI("开始监听");
    UartOpenReq ss;
    snprintf(ss.serialPort, sizeof(ss.serialPort), "/dev/ttyS1");
    ss.speed = 115200;
    ss.flow_ctrl = 0;
    ss.databits = 8;
    ss.stopbits = 1;
    ss.parity = 'N';

    mUartBTN = new UartClient(mPacket, BT_BTN, ss, "192.168.0.113", 1145, 0);
    mUartBTN->init();
    
    mPower = true;

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

    if (mUartBTN) {
        mUartBTN->onTick();
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

    snd.checkcode();    // 修改检验位

    LOG(VERBOSE) << "send to btn. bytes=" << hexstr(bd->buf, bd->len);
    mUartBTN->send(bd);
    mLastSendTime = SystemClock::uptimeMillis();
}

void BtnMgr::onCommDeal(IAck* ack) {
    if (!mPower)return;
    // LOG(VERBOSE) << "hex str=\n" << hexstr(ack->mBuf, ack->mDlen);
    int64_t now_tick = SystemClock::uptimeMillis();

    static uchar clickType = 0x00, clickValue = 0;
    uchar type = ack->getData(2), value = ack->getData(3);

    if (value == 0 && clickValue != 0 && clickType == type) {
        sendButton(clickType == 0x0B ? clickValue * 10 : clickValue, HW_EVENT_UP);
        clickType = 0x00;
        clickValue = 0;
    } else if (value != 0 && clickValue == 0) {
        sendButton(type == 0x0B ? value * 10 : value, HW_EVENT_DOWN);
        clickType = type;
        clickValue = value;
        mDownTime = now_tick;
    } else if (clickValue != 0 && now_tick - mDownTime >= 700) {
        sendButton(clickType == 0x0B ? clickValue * 10 : clickValue, HW_EVENT_LONG);
    }
}

void BtnMgr::sendButton(ushort key, uchar status) {
    LOGV("sendButton = %d", key);
    auto it = keyMap.find(key);
    if (it != keyMap.end()) {
        LOGV("key = %d status = %d", it->second, status);
        g_windMgr->sendKey(it->second, status);
    }
}