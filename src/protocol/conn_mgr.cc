#include "conn_mgr.h"
#include <core/app.h>

#include "config_mgr.h"

#include "manage.h"
#include "this_func.h"
#include "base_data.h"

#define TICK_TIME 100 // tick触发时间（毫秒）

//////////////////////////////////////////////////////////////////

CConnMgr::CConnMgr() {
    mPacket = new SDHWPacketBuffer();
    mUartMCU = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;
    mLastStatusTime = 0;
    mLastWarnTime = 0;

    mPacket->setType(BT_MCU, BT_MCU);
    CHandlerManager::ins()->addHandler(BT_MCU, this);
}

CConnMgr::~CConnMgr() {
    __del(mUartMCU);
}

int CConnMgr::init() {
    LOGI("开始监听");
    UartOpenReq ss;

    snprintf(ss.serialPort, sizeof(ss.serialPort), "/dev/ttyS3");
    ss.speed = 9600;
    ss.flow_ctrl = 0;
    ss.databits = 8;
    ss.stopbits = 1;
    ss.parity = 'N';

    mUartMCU = new UartClient(mPacket, BT_MCU, ss, "192.168.0.113", 1142, 0);
    mUartMCU->init();

    mLastAcceptTime = 0;

    // 启动延迟一会后开始发包
    mNextEventTime = SystemClock::uptimeMillis() + TICK_TIME * 10;
    App::getInstance().addEventHandler(this);
    return 0;
}

int CConnMgr::checkEvents() {
    int64_t curr_tick = SystemClock::uptimeMillis();
    if (curr_tick >= mNextEventTime) {
        mNextEventTime = curr_tick + TICK_TIME;
        return 1;
    }
    return 0;
}

int CConnMgr::handleEvents() {
    int64_t now_tick = SystemClock::uptimeMillis();

    if (mUartMCU) mUartMCU->onTick();
    send2MCU();

    return 1;
}

void CConnMgr::send2MCU() {
    BuffData* bd = mPacket->obtain(BT_MCU, 0);
    UI2MCU   snd(bd, BT_MCU);

    snd.checkcode(); // 修改检验位
    LOG(VERBOSE) << "send to mcu. bytes=" << hexstr(bd->buf, bd->len) << "    -->";
    mUartMCU->send(bd);

    mLastSendTime = SystemClock::uptimeMillis();
}

void CConnMgr::onCommDeal(IAck* ack) {
    if (mLastAcceptTime == 0)mLastAcceptTime = SystemClock::uptimeMillis();
    LOG(VERBOSE) << "accept. bytes=" << hexstr(ack->mBuf, ack->mDlen) << "        <--";

    mLastAcceptTime = SystemClock::uptimeMillis();
}