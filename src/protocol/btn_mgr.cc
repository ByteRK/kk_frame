#include "btn_mgr.h"
#include <core/app.h>

#define TICK_TIME 50 // tick触发时间（毫秒）

//////////////////////////////////////////////////////////////////

BtnMgr::BtnMgr() {
    mPacket = new SDHWPacketBuffer();
    mUartMCU = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;

    mVersionL = 0;
    mVersionR = 0;
    memset(mBtnLight, BTN_HIGHT, ALL_BTN_COUNT);
    mBtnLightChanged = true;

    mPacket->setType(BT_BTN, BT_BTN);
    CHandlerManager::ins()->addHandler(BT_BTN, this);
}

BtnMgr::~BtnMgr() {
    __del(mUartMCU);
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

    mUartMCU = new UartClient(mPacket, BT_BTN, ss, "192.168.0.113", 1134, 0);
    mUartMCU->init();

    // 启动延迟一会后开始发包
    mNextEventTime = SystemClock::uptimeMillis() + TICK_TIME * 10;
    App::getInstance().addEventHandler(this);
    return 0;
}

/// @brief 获取版本号
/// @return 
std::string BtnMgr::getVersion() {
    return                                                    \
        (mVersionL ? std::to_string(mVersionL) : "-")     \
        + "." + \
        (mVersionR ? std::to_string(mVersionR) : "-");
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
    if (!mUartMCU) return 1;
    mUartMCU->onTick();
    if (mBtnLightChanged) {
        send2MCU();
        mBtnLightChanged = false;
    }
    return 1;
}

/// @brief 发送串口消息
void BtnMgr::send2MCU() {
    BuffData* bd = mPacket->obtain(BT_BTN, 0);
    UI2MCU   snd(bd, BT_BTN);
    snd.setData(2, 0x01);
    memcpy(bd->buf + 3, mBtnLight, ALL_BTN_COUNT);
    snd.checkcode();
    LOG(VERBOSE) << "send to btn. bytes=" << hexstr(bd->buf, bd->len);
    mUartMCU->send(bd);
    mLastSendTime = SystemClock::uptimeMillis();
}

/// @brief 处理串口信息
/// @param ack 
void BtnMgr::onCommDeal(IAck* ack) {
    LOG(VERBOSE) << "hex str=" << hexstr(ack->mBuf, ack->mDlen);
}

/// @brief 设置按键灯亮度
/// @param light 
void BtnMgr::setLight(uint8_t* light) {
    for (uint8_t i = 0; i < ALL_BTN_COUNT; i++) {
        if (light[i] != mBtnLight[i]) {
            memcpy(mBtnLight, light, ALL_BTN_COUNT);
            mBtnLightChanged = true;
            break;
        }
    }
}
