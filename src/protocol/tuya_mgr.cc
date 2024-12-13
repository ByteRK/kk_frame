/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-01 03:03:02
 * @LastEditTime: 2024-11-13 16:21:58
 * @FilePath: /kaidu_t2e_pro/src/protocol/tuya_mgr.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include "tuya_mgr.h"
#include <core/app.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "config_mgr.h"
#include "hv_version.h"

#include "conn_mgr.h"
#include "manage.h"
#include "this_func.h"

#include "pop_tip.h"

#define TICK_TIME 50 // tick触发时间（毫秒）

 /**
  * 分包大小
  * 0x00：默认 256 bytes（兼容旧固件）
  * 0x01：512 bytes
  * 0x02：1024 bytes
  */
#define OTA_PACKAGE_LEVEL 0x02

#define OTA_SAVE_PATH  "/tmp/kaidu_t2e_pro.tar.gz"
#define RM_OTA_PATH  "rm -rf " OTA_SAVE_PATH

  //////////////////////////////////////////////////////////////////

TuyaMgr::TuyaMgr() {
    mPacket = new SDHWPacketBuffer();
    mUartTUYA = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;
    mLastSyncDateTime = 0;
    mLastSendDiffDPTime = 0;

    mPacket->setType(BT_TUYA, BT_TUYA);
    CHandlerManager::ins()->addHandler(BT_TUYA, this);
}

TuyaMgr::~TuyaMgr() {
    __del(mUartTUYA);
}

int TuyaMgr::init() {
    LOGI("开始监听");
    UartOpenReq ss;

    snprintf(ss.serialPort, sizeof(ss.serialPort), "/dev/ttyS2");

    ss.speed = 9600;
    ss.flow_ctrl = 0;
    ss.databits = 8;
    ss.stopbits = 1;
    ss.parity = 'N';

    mUartTUYA = new UartClient(mPacket, BT_TUYA, ss, "192.168.0.113", 1144, 0);
    mUartTUYA->init();

    mLastAcceptTime = 0;

    mIsRunConnectWork = false;
    mNetWorkConnectTime = 0;

    // 启动延迟一会后开始发包
    mNextEventTime = SystemClock::uptimeMillis() + TICK_TIME * 10;
    App::getInstance().addEventHandler(this);
    return 0;
}

int TuyaMgr::checkEvents() {
    int64_t curr_tick = SystemClock::uptimeMillis();
    if (curr_tick >= mNextEventTime) {
        mNextEventTime = curr_tick + TICK_TIME;
        return 1;
    }
    return 0;
}

int TuyaMgr::handleEvents() {
    int64_t now_tick = SystemClock::uptimeMillis();

    if (mUartTUYA) mUartTUYA->onTick();

    if (mIsRunConnectWork) {
        if (now_tick - mLastSendDiffDPTime >= 400) {
            sendDiffDp();
            mLastSendDiffDPTime = now_tick;
        }

        if (now_tick - mLastSyncDateTime >= 1080000) {
            getTuyaTime();
        }
    } else {
        if (g_data->mNetWorkDetail == 0x04 &&
            now_tick - mNetWorkConnectTime >= 1000) {
            openWeatherServe();
            // send2MCU(TYCOMM_GET_TIME);
            openTimeServe();
            mIsRunConnectWork = true;
        }
    }

    return 1;
}

void TuyaMgr::send2MCU(uint8_t cmd) {
    send2MCU(0, 0, cmd);
}

void TuyaMgr::send2MCU(uint8_t* buf, uint16_t len, uint8_t cmd) {
    BuffData* bd = mPacket->obtain(BT_TUYA, len);
    UI2MCU   snd(bd, BT_TUYA);

    snd.setData(TUYA_VERSION, 0x03);
    snd.setData(TUYA_COMM, cmd);
    snd.setData(TUYA_DATA_LEN_H, (len >> 8) & 0xFF);
    snd.setData(TUYA_DATA_LEN_L, len & 0xFF);
    snd.setData(buf, TUYA_DATA_START, len);

    snd.checkcode();    // 修改检验位
    LOG(VERBOSE) << "send to tuya. bytes=" << hexstr(bd->buf, bd->len);
    mUartTUYA->send(bd);
    mLastSendTime = SystemClock::uptimeMillis();
}

void TuyaMgr::onCommDeal(IAck* ack) {
    if (mLastAcceptTime == 0)mLastAcceptTime = SystemClock::uptimeMillis();

    bool show = false;
    switch (ack->getData(TUYA_COMM)) {
    case TYCOMM_HEART:
        sendHeartBeat();
        break;
    case TYCOMM_INFO:
        if (mClearWifi) {
            sendSetConnectMode(1);
        } else {
            sendSetConnectMode(2);
        }
        break;
    case TYCOMM_CHECK_MDOE:
        sendSetWorkMode();
        break;
    case TYCOMM_WIFI_STATUS:
        sendWIFIStatus(ack->getData(TUYA_DATA_START));
        break;
    case TYCOMM_ACCEPT: {
        uint16_t dealDpLen = 7 + ack->getData2(TUYA_DATA_LEN_H);
        acceptDP(ack->mBuf + TUYA_DATA_START, ack->getData2(TUYA_DATA_LEN_H));

        // if (ack->mDlen > dealDpLen + 7) {
        //     uint8_t* newDp = ack->mBuf + dealDpLen;
        //     uint8_t  newPacketLen = newDp[TUYA_DATA_LEN_H] << 8 | newDp[TUYA_DATA_LEN_L];
        //     if (ack->mDlen >= dealDpLen + 7 + newPacketLen) {
        //         acceptDP(newDp + TUYA_DATA_START, newDp[TUYA_DATA_LEN_H] << 8 | newDp[TUYA_DATA_LEN_L]);
        //     }
        // }
    }   break;
    case TYCOMM_CHECK:
        sendDp();
        break;
    case TYCOMM_WIFITEST:
        g_data->mWifiTestRes = ack->getData2(TUYA_DATA_START, true);
        break;

    case TYCOMM_GET_TIME:
        acceptTime(ack->mBuf + TUYA_DATA_START);
        break;
    case TYCOMM_OPEN_WEATHER:
        acceptOpenWeather(ack->mBuf + TUYA_DATA_START);
        break;
    case TYCOMM_WEATHER:
        acceptWeather(ack->mBuf + TUYA_DATA_START, ack->getData2(TUYA_DATA_LEN_H));
        break;
    case TYCOMM_OPEN_TIME:
        acceptOpenTime(ack->mBuf + TUYA_DATA_START);
        break;

    case TYCOMM_OTA_START:
        dealOTAComm(ack->mBuf + TUYA_DATA_START, ack->getData2(TUYA_DATA_LEN_H));
        break;
    case TYCOMM_OTA_DATA:
        dealOTAData(ack->mBuf + TUYA_DATA_START, ack->getData2(TUYA_DATA_LEN_H));
        break;
    default:
        show = true;
        LOG(INFO) << "[default]accept. bytes=" << hexstr(ack->mBuf, ack->mDlen);
        break;
    }

    if (!show)
        LOG(VERBOSE) << "[default]accept. bytes=" << hexstr(ack->mBuf, ack->mDlen);

    mLastAcceptTime = SystemClock::uptimeMillis();
}

void TuyaMgr::sendHeartBeat() {
    LOG(VERBOSE) << "心跳";
    static bool firstSendHeartBeat = false;
    uint8_t data[1] = { 0x01 };
    if (!firstSendHeartBeat) {
        data[0] = 0x00;
        firstSendHeartBeat = true;
    }
    send2MCU(data, 1, TYCOMM_HEART);
}

void TuyaMgr::sendWifiTest() {
    LOG(VERBOSE) << "WIFI测试";
    g_data->mWifiTestRes = 0xFFFF; // 复位
    send2MCU(TYCOMM_WIFITEST);
}

void TuyaMgr::resetWifi(bool clear) {
    LOG(VERBOSE) << "重置wifi" << clear ? "[清空]" : "[非清空]";
    mClearWifi = clear;
    g_data->mNetWorkDetail = 0;
    g_data->mNetWork = WIFI_NULL;
    uint8_t data[2] = { 0x00,0x00 };
    send2MCU(data, 2, TYCOMM_RESET);
}

void TuyaMgr::sendSetConnectMode(uint8_t mode) {
    LOG(VERBOSE) << "设置配网模式";
    std::string str = "{\"p\":\"cdwan0nqtmbyqvx3\",\"v\":\"" + std::string(HV_SOFT_VER_EXT) + "\",\"m\":" + std::to_string(mode % 10) + "}";
    LOGE("[tuyaConfig] -> %s", str.c_str());
    uint8_t data[0x2a];
    memcpy(data, str.c_str(), 0x2a);
    send2MCU(data, 0x2a, TYCOMM_INFO);
}

void TuyaMgr::sendSetWorkMode() {
    send2MCU(TYCOMM_CHECK_MDOE);
}

void TuyaMgr::sendWIFIStatus(uint8_t status) {
    LOG(VERBOSE) << "获取wifi工作状态";
    g_data->mNetWorkDetail = status;
    switch (status) {
    case 0x04:
        if (g_data->mNetWork != 0x04) {
            mIsRunConnectWork = false;
            mNetWorkConnectTime = SystemClock::uptimeMillis();
        }
        g_data->mNetWork = WIFI_4;
        break;
    case 0x02:
        g_data->mNetWork = WIFI_ERROR;
        break;
    case 0x00:
    case 0x01:
    default:
        g_data->mNetWork = WIFI_NULL;
        break;
    }
    send2MCU(TYCOMM_WIFI_STATUS);

    if (mClearWifi) { resetWifi(); mClearWifi = false; }
}

void TuyaMgr::getTuyaTime() {
    LOG(VERBOSE) << "获取涂鸦时间";
    mLastSyncDateTime = SystemClock::uptimeMillis();
    send2MCU(TYCOMM_GET_TIME);
}

void TuyaMgr::openTimeServe() {
    LOG(VERBOSE) << "开启涂鸦时间服务";
    uint8_t data[2] = { 0x01 ,0x01 };
    mLastSyncDateTime = SystemClock::uptimeMillis();
    send2MCU(data, 2, TYCOMM_OPEN_TIME);
}

void TuyaMgr::openWeatherServe() {
    LOG(VERBOSE) << "开启涂鸦天气服务";
    std::vector<std::string> strings = {
            "w.temp",
            "w.conditionNum",
            "w.thigh",
            "w.tlow",
            "w.date.1"
    };

    uint8_t totalLength = 0;
    for (const auto& str : strings) {
        totalLength += str.length() + 1;
    }

    uint8_t data[totalLength];
    uint8_t offset = 0;

    for (const auto& str : strings) {
        data[offset++] = str.length();
        memcpy(data + offset, str.c_str(), str.length());
        offset += str.length();
    }
    send2MCU(data, totalLength, TYCOMM_OPEN_WEATHER);
}

void TuyaMgr::sendDp() {
    LOG(VERBOSE) << "开机DP全上报";
    static uint8_t s_SendDpBuf[256];
    memset(s_SendDpBuf, 0, 256);
    uint16_t count = 0;

    createDp(s_SendDpBuf, count, TYDPID_POWER, TUYATYPE_BOOL, &mPower, 1);
    createDp(s_SendDpBuf, count, TYDPID_FUN_SPEED, TUYATYPE_ENUM, &mFunLevel, 1);
    createDp(s_SendDpBuf, count, TYDPID_LOCK, TUYATYPE_BOOL, &mLock, 1);
    createDp(s_SendDpBuf, count, TYDPID_LAMP, TUYATYPE_BOOL, &mLamp, 1);
    createDp(s_SendDpBuf, count, TYDPID_START, TUYATYPE_BOOL, &mStart, 1);
    createDp(s_SendDpBuf, count, TYDPID_END_DATE, TUYATYPE_VALUE, &mReserveTime, 4);
    createDp(s_SendDpBuf, count, TYDPID_ALL_TIME, TUYATYPE_VALUE, &mAllTime, 4);
    createDp(s_SendDpBuf, count, TYDPID_OVER_TIME, TUYATYPE_VALUE, &mOverTime, 4);
    createDp(s_SendDpBuf, count, TYDPID_SET_TEM, TUYATYPE_VALUE, &mSetTem, 4);
    createDp(s_SendDpBuf, count, TYDPID_NOW_TEM, TUYATYPE_VALUE, &mNowTem, 4);
    createDp(s_SendDpBuf, count, TYDPID_FAULT, TUYATYPE_BITMAP, &mFault, 4);
    createDp(s_SendDpBuf, count, TYDPID_MODE, TUYATYPE_ENUM, &mMode, 1);
    createDp(s_SendDpBuf, count, TYDPID_LAMP2, TUYATYPE_BOOL, &mLamp2, 1);
    // createDp(s_SendDpBuf, count, TYDPID_HOB_STATUS, TUYATYPE_ENUM, &mHobStatus, 1);
    createHobDp(s_SendDpBuf, count);
    createDp(s_SendDpBuf, count, TYDPID_STATUS, TUYATYPE_ENUM, &mStatus, 1);
    createDp(s_SendDpBuf, count, TYDPID_LEFT_TIMER, TUYATYPE_VALUE, &mLeftTimer, 4);
    createDp(s_SendDpBuf, count, TYDPID_RIGHT_TIMER, TUYATYPE_VALUE, &mRightTimer, 4);
    createDp(s_SendDpBuf, count, TYDPID_CLEAN, TUYATYPE_BOOL, &mClean, 1);
    createDp(s_SendDpBuf, count, TYDPID_WARM, TUYATYPE_BOOL, &mInsulation, 1);
    // createDp(s_SendDpBuf, count, TYDPID_TIMER_STATUS, TUYATYPE_ENUM, &mTimerStatus, 1);
    createDp(s_SendDpBuf, count, TYDPID_SMOKE_TIMER, TUYATYPE_VALUE, &mSmokerDisplay, 4);
    createDp(s_SendDpBuf, count, TYDPID_WARM_TIME, TUYATYPE_VALUE, &mInsulationTime, 4);
    createAirTimerDp(s_SendDpBuf, count);
    createDp(s_SendDpBuf, count, TYDPID_CLEAN_TIP, TUYATYPE_ENUM, &mCleanStatus, 1);
    createDp(s_SendDpBuf, count, TYDPID_BOOT_MODE, TUYATYPE_ENUM, &mBootMode, 1);
    // createDp(s_SendDpBuf, count, TYDPID_WATER, TUYATYPE_BOOL, &mWater, 1);
    createDp(s_SendDpBuf, count, TYDPID_SMOKE_SET, TUYATYPE_ENUM, &mSmokerDelay, 1);
    createDp(s_SendDpBuf, count, TYDPID_SMOKE_RUN, TUYATYPE_BOOL, &mSmokerRun, 1);
    createDp(s_SendDpBuf, count, TYDPID_FUN_AUTO, TUYATYPE_BOOL, &mAuto, 1);
    createDp(s_SendDpBuf, count, TYDPID_ALART, TUYATYPE_BITMAP, &mAlart, 1);

    mLastSendDiffDPTime = SystemClock::uptimeMillis();
    send2MCU(s_SendDpBuf, count, TYCOMM_SEND);
}

void TuyaMgr::sendDiffDp() {
    static uint8_t s_SendDiffDpBuf[256];
    memset(s_SendDiffDpBuf, 0, 256);
    uint16_t count = 0;

    if (g_data->mTUYAPower != mPower) {
        mPower = g_data->mTUYAPower;
        createDp(s_SendDiffDpBuf, count, TYDPID_POWER, TUYATYPE_BOOL, &mPower, 1);
    }

    uint8_t funLevel = g_data->mFunLevel == 5 ? g_data->mFunLevel2 : g_data->mFunLevel;
    if (funLevel != mFunLevel) {
        mFunLevel = funLevel;
        createDp(s_SendDiffDpBuf, count, TYDPID_FUN_SPEED, TUYATYPE_ENUM, &mFunLevel, 1);
    }

    if (g_data->mLock != mLock) {
        mLock = g_data->mLock;
        createDp(s_SendDiffDpBuf, count, TYDPID_LOCK, TUYATYPE_BOOL, &mLock, 1);
    }

    if (g_data->mLamp != mLamp) {
        mLamp = g_data->mLamp;
        createDp(s_SendDiffDpBuf, count, TYDPID_LAMP, TUYATYPE_BOOL, &mLamp, 1);
    }

    bool start = g_data->mStatus == STATUS_PREHEAT || g_data->mStatus == STATUS_WORKING;
    if (start != mStart) {
        mStart = start;
        createDp(s_SendDiffDpBuf, count, TYDPID_START, TUYATYPE_BOOL, &mStart, 1);
    }

    if (g_data->mStatus == STATUS_RESERVE) {
        uint32_t reserveTime = (g_data->mReserveTime.tm_hour * 60 + g_data->mReserveTime.tm_min + g_data->mWorkAllTime / 60) % 1440;
        if (reserveTime != mReserveTime) {
            mReserveTime = reserveTime;
            createDp(s_SendDiffDpBuf, count, TYDPID_END_DATE, TUYATYPE_VALUE, &mReserveTime, 4);
        }
    }

    uint32_t workAllTime = g_data->mWorkAllTime / 60;
    if (g_data->mRunType == RUN_DEFAULT && workAllTime != mAllTime) {
        mAllTime = workAllTime;
        createDp(s_SendDiffDpBuf, count, TYDPID_ALL_TIME, TUYATYPE_VALUE, &mAllTime, 4);
    }

    uint32_t overTime = (g_data->mOverTime + 59000) / 60000;
    if (overTime != mOverTime && overTime != 0) {
        mOverTime = overTime;
        createDp(s_SendDiffDpBuf, count, TYDPID_OVER_TIME, TUYATYPE_VALUE, &mOverTime, 4);
    }

    if (g_data->mRunType == RUN_DEFAULT && g_data->mUpTem != mSetTem) {
        mSetTem = g_data->mUpTem;
        createDp(s_SendDiffDpBuf, count, TYDPID_SET_TEM, TUYATYPE_VALUE, &mSetTem, 4);
    }

    uint32_t nowTem = g_data->nUpTem > g_data->mUpTem ? g_data->mUpTem : g_data->nUpTem;
    if (start && nowTem != mNowTem) {
        mNowTem = nowTem;
        createDp(s_SendDiffDpBuf, count, TYDPID_NOW_TEM, TUYATYPE_VALUE, &mNowTem, 4);
    }

    uint32_t error = g_data->nError & ERROR_TYPE_APP;
    if (g_window->getPopType() == POP_TIP)
        error |= __dc(PopTip, g_window->getPop())->getTipError(); // 故障弹窗
    if (error != mFault) {
        mFault = error;
        createDp(s_SendDiffDpBuf, count, TYDPID_FAULT, TUYATYPE_BITMAP, &mFault, 4);
    }

    uint8_t mode = g_data->mRunType == RUN_SMART ? MODE_ADDFUNS_ONEKEY :
        g_data->mRunType == RUN_DIY ? MODE_EMUN_DIY : g_data->mMode;
    if (mode != mMode) {
        if (mode == MODE_ADDFUNS_ONEKEY) mRecipe = 0;
        mMode = mode;
        createDp(s_SendDiffDpBuf, count, TYDPID_MODE, TUYATYPE_ENUM, &mMode, 1);
    }

    if (g_data->mLamp2 != mLamp2) {
        mLamp2 = g_data->mLamp2;
        createDp(s_SendDiffDpBuf, count, TYDPID_LAMP2, TUYATYPE_BOOL, &mLamp2, 1);
    }

    uint8_t lTimerStatus = g_data->mTimerOverTime[0] <= 1000 ? 2 : g_data->mTimerOverTime[0] <= 3000 ? 1 : 0;
    uint8_t rTimerStatus = g_data->mTimerOverTime[1] <= 1000 ? 2 : g_data->mTimerOverTime[1] <= 3000 ? 1 : 0;
    if (g_data->getHobStatus(-1) != mLHobStatus ||
        g_data->getHobStatus(1) != mRHobStatus ||
        lTimerStatus != mLTimerStatus ||
        rTimerStatus != mRTimerStatus) {
        mLHobStatus = g_data->getHobStatus(-1);
        mRHobStatus = g_data->getHobStatus(1);
        mLTimerStatus = lTimerStatus;
        mRTimerStatus = rTimerStatus;
        createHobDp(s_SendDiffDpBuf, count);

        // createDp(s_SendDiffDpBuf, count, TYDPID_HOB_STATUS, TUYATYPE_ENUM, &mHobStatus, 1);
        // createDp(s_SendDiffDpBuf, count, TYDPID_TIMER_STATUS, TUYATYPE_ENUM, &mTimerStatus, 1);
    }

    uint8_t status = 0; // 等待
    if (g_data->mStatus == STATUS_STANDBY && g_window->getPageType() == PAGE_RUN) {
        status = 3;     // 完成
    } else {
        switch (g_data->mStatus) {
        case STATUS_RESERVE:
            status = 1; // 预约中
            break;
        case STATUS_WORKING:
            status = 2; // 工作中
            break;
        case STATUS_STOP:
        case STATUS_PREHEAT_STOP:
            status = 4; // 暂停中
            break;
        case STATUS_PREHEAT:
            status = 5;
            break;
        case STATUS_PREHEAT_FINAL:
            status = 6; // 预热保温
            break;
        }
    }
    bool statusChange = status != mStatus;
    if (statusChange) {
        mStatus = status;
        createDp(s_SendDiffDpBuf, count, TYDPID_STATUS, TUYATYPE_ENUM, &mStatus, 1);
    }

    uint32_t leftTimer = (g_data->mTimerOverTime[0] + RUN_TIME_SUPPLEMENT_MS) / 60000;
    if (leftTimer != mLeftTimer) {
        mLeftTimer = leftTimer;
        createDp(s_SendDiffDpBuf, count, TYDPID_LEFT_TIMER, TUYATYPE_VALUE, &mLeftTimer, 4);
    }

    uint32_t rightTimer = (g_data->mTimerOverTime[1] + RUN_TIME_SUPPLEMENT_MS) / 60000;
    if (rightTimer != mRightTimer) {
        mRightTimer = rightTimer;
        createDp(s_SendDiffDpBuf, count, TYDPID_RIGHT_TIMER, TUYATYPE_VALUE, &mRightTimer, 4);
    }

    if (g_data->mClean != mClean) {
        mClean = g_data->mClean;
        createDp(s_SendDiffDpBuf, count, TYDPID_CLEAN, TUYATYPE_BOOL, &mClean, 1);
    }

    if (g_data->mInsulation != mInsulation) {
        mInsulation = g_data->mInsulation;
        createDp(s_SendDiffDpBuf, count, TYDPID_WARM, TUYATYPE_BOOL, &mInsulation, 1);
    }

    if (mMode == MODE_ADDFUNS_ONEKEY) {
        if (g_data->mSelectSmart + 1 != mRecipe) {
            mRecipe = g_data->mSelectSmart + 1;
            createDp(s_SendDiffDpBuf, count, TYDPID_RECIPE, TUYATYPE_VALUE, &mRecipe, 4);
        }
    }

    // if (g_data->mMode == MODE_BAKED_BAKING) { // 独立烘培才上报上下管
    //     if (g_data->mDownTem != mDownTem) {
    //         mDownTem = g_data->mDownTem;
    //         createDp(s_SendDiffDpBuf, count, TYDPID_DOWN_TEM, TUYATYPE_VALUE, &mDownTem, 4);
    //     }
    //     if (g_data->mUpTem != mUpTem) {
    //         mUpTem = g_data->mUpTem;
    //         createDp(s_SendDiffDpBuf, count, TYDPID_UP_TEM, TUYATYPE_VALUE, &mUpTem, 4);
    //     }
    // }

    uint32_t smokerTimer = g_data->mDelayOverTime > 0 ? (g_data->mDelayOverTime + RUN_TIME_SUPPLEMENT_MS) / 60000 : 0;
    LOGV("smokerTimer = %d, mSmokerDisplay = %d", smokerTimer, mSmokerDisplay);
    if (smokerTimer != mSmokerDisplay) {
        mSmokerDisplay = smokerTimer;
        createDp(s_SendDiffDpBuf, count, TYDPID_SMOKE_TIMER, TUYATYPE_VALUE, &mSmokerDisplay, 4);
    }

    if (mMode == MODE_EMUN_DIY) {
        if (g_data->mNowStep != mDiyStep || statusChange || mDiyChange) {
            mDiyChange = false;
            mDiyStep = g_data->mNowStep;
            createDIYDp(s_SendDiffDpBuf, count);
        }
    } else {
        mDiyStep = -1;
    }

    uint64_t insulationTime = g_data->mInsulationTime / 60000;
    if (insulationTime != mInsulationTime) {
        mInsulationTime = insulationTime;
        if (g_data->mInsulation)
            createDp(s_SendDiffDpBuf, count, TYDPID_WARM_TIME, TUYATYPE_VALUE, &mInsulationTime, 4);
    }

    for (uint8_t i = 0;i < 5;i++) {
        if (g_data->mAirTimer[i] != mAirTime[i]) {
            LOGI("mAirTime[%d] = %d, g_data->mAirTimer[%d] = %d", i, mAirTime[i], i, g_data->mAirTimer[i]);
            memcpy(mAirTime, g_data->mAirTimer, 5);
            createAirTimerDp(s_SendDiffDpBuf, count);
            break;
        }
    }

    uint8_t cleanLevel = 0;
    if (g_data->mFunRunAllTime >= 54000000) cleanLevel = 2;
    else if (g_data->mFunRunAllTime >= 36000000) cleanLevel = 1;
    if (cleanLevel != mCleanStatus) {
        mCleanStatus = cleanLevel;
        createDp(s_SendDiffDpBuf, count, TYDPID_CLEAN_TIP, TUYATYPE_ENUM, &mCleanStatus, 1);
    }

    uint8_t autoFunLevel = (g_data->mAutoFunLevel + 3) % 4;
    if (autoFunLevel != mBootMode) {
        mBootMode = autoFunLevel;
        createDp(s_SendDiffDpBuf, count, TYDPID_BOOT_MODE, TUYATYPE_ENUM, &mBootMode, 1);
    }

    if (g_data->mDelayLevel != mSmokerDelay) {
        mSmokerDelay = g_data->mDelayLevel;
        createDp(s_SendDiffDpBuf, count, TYDPID_SMOKE_SET, TUYATYPE_ENUM, &mSmokerDelay, 1);
    }

    bool delayRun = g_data->mDelayOverTime > 0;
    if (delayRun != mSmokerRun) {
        mSmokerRun = delayRun;
        createDp(s_SendDiffDpBuf, count, TYDPID_SMOKE_RUN, TUYATYPE_BOOL, &mSmokerRun, 1);
    }

    if ((g_data->mFunLevel == 5) != mAuto) {
        mAuto = g_data->mFunLevel == 5;
        createDp(s_SendDiffDpBuf, count, TYDPID_FUN_AUTO, TUYATYPE_BOOL, &mAuto, 1);
    }

    uint8_t alart = 0;
    if (g_data->nError & ERROR_QUESHUI ||
        (g_window->getPopType() == POP_TIP && __dc(PopTip, g_window->getPop())->getTipError() & ERROR_QUESHUI)
        ) alart |= 0b1; // 缺水
    if (g_data->mZFPRunAllTime >= 360000000) alart |= 0b10; // 除垢
    if (g_data->nDoor) alart |= 0b1000;
    if (alart != mAlart) {
        mAlart = alart;
        createDp(s_SendDiffDpBuf, count, TYDPID_ALART, TUYATYPE_BITMAP, &mAlart, 1);
    }

    if (count == 0)return;
    LOG(VERBOSE) << "DP差异上报";
    send2MCU(s_SendDiffDpBuf, count, TYCOMM_SEND);
}

uint8_t TuyaMgr::getOTAProgress() {
    if (mOTALen == 0)return 0;
    if (mOTACurLen >= mOTALen)return 100;
    return (mOTACurLen * 100) / mOTALen;
}

uint64_t TuyaMgr::getOTAAcceptTime() {
    return mOTAAcceptTime;
}

void TuyaMgr::createDp(uint8_t* buf, uint16_t& count, uint8_t dp, uint8_t type, void* data, uint16_t dlen, bool reverse) {
    uint8_t* ui8Data = static_cast<uint8_t*>(data);
    buf[count + 0] = dp;
    buf[count + 1] = type;
    buf[count + 2] = (dlen >> 8) & 0xFF;
    buf[count + 3] = dlen & 0xFF;

    if (reverse) {
        // 逆序复制 ui8Data 到 buf + count + 4
        for (int i = 0; i < dlen; ++i) buf[count + 4 + i] = ui8Data[dlen - 1 - i];
    } else {
        // 正序复制 ui8Data 到 buf + count + 4
        memcpy(buf + count + 4, ui8Data, dlen);
    }
    count += (4 + dlen);
}

void TuyaMgr::createDIYDp(uint8_t* buf, uint16_t& count) {
    uint8_t diyBuf[128];
    uint8_t diyLen = 0;
    uint8_t size = g_data->mDIYMode.modes.size();
    diyBuf[0] = g_data->mDIYMode.id;
    diyBuf[1] = g_data->mStatus == STATUS_RESERVE ? 0 : g_data->mNowStep + 1;
    diyBuf[2] = size;
    diyLen = 3;

    for (uint8_t i = 0; i < size; i++) {
        diyBuf[diyLen + 0] = i;                                            // 编号
        diyBuf[diyLen + 1] = g_data->mDIYMode.modes[i].mode;               // 模式
        diyBuf[diyLen + 2] = g_data->mDIYMode.modes[i].tem >> 8 & 0xFF;    // 上温高
        diyBuf[diyLen + 3] = g_data->mDIYMode.modes[i].tem & 0xFF;         // 上温低
        diyBuf[diyLen + 4] = g_data->mDIYMode.modes[i].tem >> 24 & 0xFF;   // 下温高
        diyBuf[diyLen + 5] = g_data->mDIYMode.modes[i].tem >> 16 & 0xFF;   // 下温低
        diyBuf[diyLen + 6] = g_data->mDIYMode.modes[i].time / 60;          // 小时
        diyBuf[diyLen + 7] = g_data->mDIYMode.modes[i].time % 60;          // 分钟

        diyLen += 8;
    }

    buf[count + 0] = TYDPID_DIY;
    buf[count + 1] = TUYATYPE_RAW;
    buf[count + 2] = (diyLen >> 8) & 0xFF;
    buf[count + 3] = diyLen & 0xFF;
    memcpy(buf + count + 4, diyBuf, diyLen);
    count += (4 + diyLen);
}

void TuyaMgr::createAirTimerDp(uint8_t* buf, uint16_t& count) {
    buf[count + 0] = TYDPID_AIR_TIMER;
    buf[count + 1] = TUYATYPE_RAW;
    buf[count + 2] = 0;
    buf[count + 3] = 5;
    memcpy(buf + count + 4, g_data->mAirTimer, 5);
    count += (4 + 5);
}

void TuyaMgr::createHobDp(uint8_t* buf, uint16_t& count) {
    uint8_t hobStatus[2] = { 0,0 };
    buf[count + 0] = TYDPID_HOB_STATUS;
    buf[count + 1] = TUYATYPE_RAW;
    buf[count + 2] = 0;
    buf[count + 3] = 2;

    // 左灶具状态
    if (mLHobStatus) {
        hobStatus[0] |= 0b00000001;
        if (mLTimerStatus == 0)hobStatus[0] |= 0b00000010;
        if (mLTimerStatus == 1)hobStatus[0] |= 0b00000100;
    }

    // 右灶具状态
    if (mRHobStatus) {
        hobStatus[1] |= 0b00000001;
        if (mRTimerStatus == 0)hobStatus[1] |= 0b00000010;
        if (mRTimerStatus == 1)hobStatus[1] |= 0b00000100;
    }

    memcpy(buf + count + 4, hobStatus, 2);
    count += (4 + 2);
}

void TuyaMgr::acceptDP(uint8_t* data, uint16_t len) {
    LOGE("len = %d", len);
    LOG(WARN) << "accept dp[" << fillLength(data[0], 3) << "]. bytes=" << hexstr(data, len);
    uint16_t dealCount = 0;

    bool run = false;
    switch (g_data->mStatus) {
    case STATUS_STOP:
    case STATUS_WORKING:
    case STATUS_PREHEAT:
    case STATUS_PREHEAT_STOP:
    case STATUS_PREHEAT_FINAL:
    case STATUS_RESERVE:
        run = true;
        break;
    }

    do {
        // 关机状态下，仅处理开机指令
        if (!g_data->mTUYAPower && data[dealCount] != TYDPID_POWER) {
            break;
        }

        switch (data[dealCount]) {
        case TYDPID_POWER: {
            if (data[TUYADP_DATA]) {
                g_windMgr->mWindow->hideBlack();
            } else {
                g_data->mLock = false;
                g_data->clearWorkData(true, true);
                g_windMgr->goTo(PAGE_STANDBY, true);
            }
        }   break;

        case TYDPID_LAMP: {
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->setLamp(data[TUYADP_DATA]);
        }   break;
        case TYDPID_LAMP2: {
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->setLamp2(data[TUYADP_DATA]);
        }   break;

        case TYDPID_CLEAN: {
            if (data[TUYADP_DATA] == g_data->mClean)break;
            if (data[TUYADP_DATA]) {
                g_data->clearWorkData(true, true);
                g_data->mClean = true;
                g_windMgr->goTo(PAGE_CLEAN);
            } else {
                g_data->mClean = false;
            }
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
        }   break;
        case TYDPID_START:
            if (data[TUYADP_DATA]) {
                g_window->removePop(); // 启动清除弹窗
                if (g_data->mStatus == STATUS_STOP)g_data->mStatus = STATUS_WORKING;
                if (g_data->mStatus == STATUS_PREHEAT_STOP)g_data->mStatus = STATUS_PREHEAT;
                if (g_data->mStatus == STATUS_PREHEAT_FINAL) {
                    g_data->mStatus = STATUS_WORKING;
                    g_windMgr->goTo(PAGE_STANDBY);
                } else {
                    g_windMgr->reload();
                }
                if (run) g_data->setBuzzer(BUZZER_DEFAULT, 1);
                mStartCache = !run;
            } else {
                if (g_data->mStatus == STATUS_WORKING)g_data->mStatus = STATUS_STOP;
                if (g_data->mStatus == STATUS_PREHEAT)g_data->mStatus = STATUS_PREHEAT_STOP;
                g_windMgr->reload();
                g_data->setBuzzer(BUZZER_DEFAULT, 1);
                mStartCache = false;
            }
            break;
        case TYDPID_SET_TEM:
            if (run) {
                if (g_data->mRunType != RUN_DEFAULT)break;
                g_data->mUpTem = uint8_t_to_uint32_t(data + TUYADP_DATA);
                g_data->mDownTem = g_data->mUpTem;
                g_windMgr->getNowPage()->reload();
                mSetTemCache = 0;
                g_data->setBuzzer(BUZZER_DEFAULT, 1);
            } else {
                mSetTemCache = uint8_t_to_uint32_t(data + TUYADP_DATA);
            }
            break;
        case TYDPID_END_DATE:
            if (run) {
                if (g_data->mStatus == STATUS_RESERVE) {
                    mReserveTimeReset = uint8_t_to_uint32_t(data + TUYADP_DATA);
                    checkTuyaReserveReset();
                    g_data->setBuzzer(BUZZER_DEFAULT, 1);
                }
            } else {
                mReserveTimeCache = uint8_t_to_uint32_t(data + TUYADP_DATA);
            }
            break;
        case TYDPID_ALL_TIME: {
            if (run) {
                if (g_data->mRunType != RUN_DEFAULT)break;

                uint64_t workAllTime = uint8_t_to_uint32_t(data + TUYADP_DATA) * 60;
                if (g_data->mStatus == STATUS_RESERVE) {
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);  // 当前时间戳
                    std::time_t target_c = std::mktime(&g_data->mReserveTime);     // 当前预约开始时间戳

                    if (workAllTime > g_data->mWorkAllTime) { // 工作时间增加
                        if (workAllTime - g_data->mWorkAllTime > target_c - now_c) { // 判断当前时间是否足够启动工作
                            g_windMgr->mWindow->showPopText("设置的结束时间不足以完成工作", 1);
                            break;
                        } else {
                            target_c -= (workAllTime - g_data->mWorkAllTime);
                        }
                    } else if (workAllTime < g_data->mWorkAllTime) { // 工作时间减少
                        target_c += (g_data->mWorkAllTime - workAllTime);
                    }

                    g_data->mWorkAllTime = workAllTime;
                    g_data->mOverTime = workAllTime * 1000;
                    g_data->mReserveTime = std::move(*std::localtime(&target_c));
                    g_data->setBuzzer(BUZZER_DEFAULT, 1);
                } else {
                    g_data->mWorkAllTime = workAllTime;
                    g_data->mOverTime = workAllTime * 1000;
                }

                g_windMgr->reload();
                mAllTimeCache = 0;
            } else {
                mAllTimeCache = uint8_t_to_uint32_t(data + TUYADP_DATA);
            }
        }   break;
        case TYDPID_STATUS:
            switch (data[TUYADP_DATA]) {
            case 0:  // 烹饪等待
                g_windMgr->goTo(PAGE_STANDBY);
                g_windMgr->mWindow->hideBlack();
                break;
            default:
                break;
            }
            mStatusCache = data[TUYADP_DATA];
            break;
        case TYDPID_MODE:
            if (!run) {
                mModeCache = data[TUYADP_DATA];
            }
            break;
        case TYDPID_RECIPE:
            if (!run) {
                mRecipeCache = uint8_t_to_uint32_t(data + TUYADP_DATA);
            }
            break;
        case TYDPID_DIY:
            dealDiy(data, len, run);
            break;
        case TYDPID_BOOT_MODE:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->mAutoFunLevel = (data[TUYADP_DATA] + 1) % 4;
            break;
        case TYDPID_SMOKE_SET:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->mDelayLevel = data[TUYADP_DATA];
            break;
        case TYDPID_WARM:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->mInsulation = data[TUYADP_DATA];
            g_data->mInsulationTime = 0;
            break;
        case TYDPID_FUN_SPEED:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->setFunLevel(data[TUYADP_DATA]);
            break;
        case TYDPID_FUN_AUTO:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            g_data->setFunLevel(data[TUYADP_DATA] ? 5 : 0);
            break;
        case TYDPID_AIR_TIMER:
            g_data->setBuzzer(BUZZER_DEFAULT, 1);
            memcpy(g_data->mAirTimer, data + TUYADP_DATA, 5);
            break;

        case TYDPID_LEFT_TIMER:
            if (g_data->getHobStatus(-1)) {
                g_data->setBuzzer(BUZZER_DEFAULT, 1);
                g_data->mTimerOverTime[0] = uint8_t_to_uint32_t(data + TUYADP_DATA) * 60000;
                g_data->mTimer[0] = true;
            }
            if (!g_data->mTimerOverTime[0])g_data->mTimer[0] = false;
            break;
        case TYDPID_STOP:
            if (run) {
                switch (g_data->mStatus) {
                case STATUS_STOP:
                case STATUS_WORKING:
                    g_data->mStatus = STATUS_STANDBY;
                    if (g_window->getPageType() == PAGE_RUN)g_windMgr->goTo(PAGE_STANDBY);
                    break;
                case STATUS_PREHEAT:
                case STATUS_PREHEAT_STOP:
                case STATUS_PREHEAT_FINAL:
                    g_data->mStatus = STATUS_STANDBY;
                    if (g_window->getPageType() == PAGE_PREHEAT)g_windMgr->goTo(PAGE_STANDBY);
                    break;
                case STATUS_RESERVE:
                    g_data->mStatus = STATUS_STANDBY;
                    if (g_window->getPageType() == PAGE_RESERVE)g_windMgr->goTo(PAGE_STANDBY);
                    break;
                default:
                    break;
                }
                g_data->setBuzzer(BUZZER_DEFAULT, 1);
            }
            break;
        case TYDPID_RIGHT_TIMER:
            if (g_data->getHobStatus(1)) {
                g_data->setBuzzer(BUZZER_DEFAULT, 1);
                g_data->mTimerOverTime[1] = uint8_t_to_uint32_t(data + TUYADP_DATA) * 60000;
                g_data->mTimer[1] = true;
            }
            if (!g_data->mTimerOverTime[1])g_data->mTimer[1] = false;
            break;
        default:
            break;
        }
        dealCount += (4 + ((uint16_t)data[dealCount + TUYADP_LEN_H] << 8 | data[dealCount + TUYA_DATA_LEN_L]));
    } while (false && len - dealCount > 0);


    if (!run)checkTuyaRun();
    LOGI("final Deal Tuya Dp");
}

void TuyaMgr::acceptTime(uint8_t* data) {
    if (!data[0])return; // 消息错误
    timeSet(data[1] + 2000, data[2], data[3], data[4], data[5], 0);
}

void TuyaMgr::acceptOpenWeather(uint8_t* data) {
    if (data[0])LOGI("Weather Serve Open Success");
    else LOGE("Weather Serve Open Failed!!!  -> code: %d", data[1]);
}

void TuyaMgr::acceptWeather(uint8_t* data, uint16_t len) {
    LOG(VERBOSE) << "accept weather. bytes=" << hexstr(data, len);
    if (len > 1) {
        uint16_t dealCount = 1;
        while (len - dealCount > 0) {
            uint8_t valuelen = data[dealCount];
            std::string name;
            for (uint8_t i = 0;i < valuelen;i++)name += data[dealCount + 1 + i];
            dealCount += (1 + valuelen);

            std::string valueStr = "";
            uint32_t    valueInt = 0;

            if (data[dealCount]) { // 字符串
                valuelen = data[dealCount + 1];
                for (uint8_t i = 0;i < valuelen;i++)valueStr += data[dealCount + 2 + i];
            } else { // 整型
                valuelen = data[dealCount + 1];
                for (uint8_t i = 0;i < valuelen;i++)
                    valueInt = (valueInt << 8) | data[dealCount + 2 + i];
            }
            LOGI("name: %s  value: %s  int: %d", name.c_str(), valueStr.c_str(), valueInt);
            dealCount += (2 + valuelen);

            if (name == "w.temp" || name == "w.temp.0") {
                g_data->mTUYATem = valueInt;
            } else if (name == "w.conditionNum" || name == "w.conditionNum.0") {
                g_data->mTUYAWeather = valueStr;
            } else if (name == "w.thigh" || name == "w.thigh.0") {
                g_data->mTUYATemMax = valueInt;
            } else if (name == "w.tlow" || name == "w.tlow.0") {
                g_data->mTUYATemMin = valueInt;
            }
        }
    }
    send2MCU(TYCOMM_WEATHER);
}

void TuyaMgr::acceptOpenTime(uint8_t* data) {
    switch (data[0]) {
    case 0x01:
        if (data[1])getTuyaTime();
        break;
    case 0x02:
        if (!data[1])break;
        timeSet(data[2] + 2000, data[3], data[4], data[5], data[6], data[7]);
        break;
    }
}

void TuyaMgr::dealDiy(uint8_t* data, uint16_t len, bool isRunning) {
    uint8_t* _data = data + 4;
    if (isRunning) {
        g_data->mDIYMode.modes.clear();
        for (uint8_t stepCount = _data[2], i = 0;i < stepCount;i++) {
            uint16_t upTem = (_data[3 + 2 + i * 8] << 8) | _data[3 + 3 + i * 8];
            uint16_t downTem = (_data[3 + 4 + i * 8] << 8) | _data[3 + 5 + i * 8];
            uint16_t time = _data[3 + 6 + i * 8] * 60 + _data[3 + 7 + i * 8];

            g_data->mDIYMode.modes.push_back(SmartItemStruct(_data[4 + i * 8], downTem << 16 | upTem, time));
        }
        LOGE("id:%d  stepCount:%d  nowStep:%d", mDiyCache.id, mDiyCache.modes.size(), g_data->mNowStep);
        for (auto item : mDiyCache.modes) {
            LOGE("mode:%d  Tem:%x  time:%d", item.mode, item.tem, item.time);
        }

        if (g_data->mNowStep >= g_data->mDIYMode.modes.size())g_data->mNowStep = 0;

        g_data->mUpTem = g_data->mDIYMode.modes[g_data->mNowStep].tem & 0xffff;
        g_data->mDownTem = (g_data->mDIYMode.modes[g_data->mNowStep].tem >> 16) & 0xffff;
        g_data->mWorkAllTime = g_data->mDIYMode.getWorkTime() * 60;
        g_data->mOverTime = g_data->mDIYMode.modes[g_data->mNowStep].time * 60000;

        g_data->setBuzzer(BUZZER_DEFAULT, 1);
        g_windMgr->reload();
        mDiyChange = true;
    } else {
        mDiyCache.id = _data[0];
        mDiyCache.icon = "@mipmap/img/img2_diy";
        if (mDiyCache.id == 0)mDiyCache.name = "云菜谱";

        mDiyCache.modes.clear();
        for (uint8_t stepCount = _data[2], i = 0;i < stepCount;i++) {
            uint16_t upTem = (_data[3 + 2 + i * 8] << 8) | _data[3 + 3 + i * 8];
            uint16_t downTem = (_data[3 + 4 + i * 8] << 8) | _data[3 + 5 + i * 8];
            uint16_t time = _data[3 + 6 + i * 8] * 60 + _data[3 + 7 + i * 8];

            mDiyCache.modes.push_back(SmartItemStruct(_data[4 + i * 8], downTem << 16 | upTem, time));
        }
        LOGE("id:%d  stepCount:%d", mDiyCache.id, mDiyCache.modes.size());
        for (auto item : mDiyCache.modes) {
            LOGE("mode:%d  Tem:%x  time:%d", item.mode, item.tem, item.time);
        }
    }
}

void TuyaMgr::resetTuyaRun() {
    mStartCache = false;
    mSetTemCache = 0;
    mReserveTimeCache = -1;
    mAllTimeCache = 0;
    mStatusCache = 0;
    mModeCache = 0;
    mRecipeCache = 0;
    mDiyCache.reset();
    mUpTemCache = 0;
    mDownTemCache = 0;
}

void TuyaMgr::checkTuyaRun() {
    LOGI("checkTuyaRun");
    LOGE(
        "start:%d  tem:%d  reserve:%d allTime:%d  status:%d  mode:%d  recipe:%d  upTem:%d  downTem:%d",
        mStartCache, mSetTemCache, mReserveTimeCache, mAllTimeCache, mStatusCache, mModeCache, mRecipeCache, mUpTemCache, mDownTemCache
    );

    bool isTrue = false;
    if (mStartCache || mStatusCache == 2) {
        // mReserveTimeCache = 0;
        isTrue = checkTuyaRunData();
    } else if (mReserveTimeCache >= 0) {
        // mStartCache = false;
        isTrue = checkTuyaRunData(false, true);

        if (isTrue) {
            // 当前时间
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_c);

            uint8_t hour = mReserveTimeCache / 60;
            uint8_t minute = mReserveTimeCache % 60;
            bool today = (hour > now_tm.tm_hour || (hour == now_tm.tm_hour && minute > now_tm.tm_min));

            // 计算结束时间并构建为时间戳
            std::tm target_tm = now_tm;
            target_tm.tm_mday += !today;
            target_tm.tm_hour = hour;
            target_tm.tm_min = minute;
            target_tm.tm_sec = 0;

            // 构建开始工作时间
            std::time_t target_time = std::mktime(&target_tm);
            target_tm.tm_hour -= g_data->mWorkAllTime / 3600;
            target_tm.tm_min -= (g_data->mWorkAllTime % 3600) / 60;
            target_time = std::mktime(&target_tm);

            LOGE("target_time: %s", std::ctime(&target_time));

            g_data->mStatus = STATUS_RESERVE;
            g_data->mReserveTime = std::move(*std::localtime(&target_time));
            g_windMgr->goTo(PAGE_RESERVE);
        }

        switch (g_data->mRunType) {
        case RUN_SMART:
            if (g_data->getSelectSmartMode().preheat)
                g_windMgr->mWindow->showPopText("预约模式下机器将自动跳过预热过程", 3);
            break;
        case RUN_DIY:
            break;
        default:
            ModeStruct mode;
            g_data->getMode(mModeCache, mode);
            if (mode.preheat)
                g_windMgr->mWindow->showPopText("预约模式下机器将自动跳过预热过程", 3);
            break;
        }
    }

    if (isTrue) {
        mDiyChange = false;
        g_data->mNowStep = 0;
        g_data->mPower = true;
        if (g_data->mStatus != STATUS_RESERVE)
            g_data->setLamp2(true);
        g_data->setBuzzer(BUZZER_DEFAULT, 1);
        resetTuyaRun();

        g_windMgr->goTo(PAGE_STANDBY);
    }
}

bool TuyaMgr::checkTuyaRunData(bool setStatus, bool isReserve) {
    bool isTrue = false;
    switch (mModeCache) {
    case MODE_EMUN_DIY: // DIY
        if (!mDiyCache.isTrue())break;//DIY数据不齐
        if (isReserve && mDiyCache.getWorkTime() >= 1440)break; // 烹饪时间过长
        isTrue = true;

        g_data->mDIYMode = mDiyCache;
        g_data->setWorkData(
            RUN_DIY,
            mDiyCache.modes.at(0).mode,
            mDiyCache.name,
            mDiyCache.modes.at(0).tem & 0xFF,
            (mDiyCache.modes.at(0).tem >> 16) & 0xFF,
            mDiyCache.getWorkTime() * 60,
            mDiyCache.modes.at(0).time * 60000
        );

        g_data->mSelectSmart = 0;
        g_data->mNowSelectMenu = MENU_ADDFUNS;
        if (setStatus) g_data->mStatus = STATUS_WORKING;
        break;
    case MODE_ADDFUNS_ONEKEY: {
        if (!mRecipeCache || !g_data->isTrueSmartMode(mRecipeCache - 1))break;//缺少菜谱id
        isTrue = true;

        g_data->mSelectSmart = mRecipeCache - 1;
        g_data->mNowSelectMenu = MENU_ADDFUNS;
        const SmartStruct& mode = g_data->getSelectSmartMode();
        g_data->setWorkData(
            RUN_SMART,
            mode.modes.at(0).mode,
            mode.name,
            mode.modes.at(0).tem & 0xFF,
            (mode.modes.at(0).tem >> 16) & 0xFF,
            mode.getWorkTime() * 60,
            mode.modes.at(0).time * 60000
        );

        if (setStatus) g_data->mStatus = mode.preheat ? STATUS_PREHEAT : STATUS_WORKING;
    }   break;
    case MODE_ADDFUNS_DESCALE: {
        ModeStruct mode;
        if (!g_data->getMode(mModeCache, mode))break;//本地数据缺失
        isTrue = true;

        g_data->setWorkData(
            RUN_DESCALE,
            mode.id,
            mode.name,
            mode.tem & 0xFF,
            (mode.tem >> 8) & 0xFF,
            mode.time * 60
        );

        g_data->mNowSelectMenu = mode.parent;
        if (setStatus) g_data->mStatus = mode.preheat ? STATUS_PREHEAT : STATUS_WORKING;
    }   break;
    default: {
        ModeStruct mode;
        if (!mSetTemCache || !mAllTimeCache || !g_data->getMode(mModeCache, mode))break;//缺少温度和时间或者找不到模式
        isTrue = true;

        g_data->setWorkData(
            RUN_DEFAULT,
            mode.id,
            mode.name,
            mSetTemCache,
            mSetTemCache,
            mAllTimeCache * 60
        );

        g_data->mNowSelectMenu = mode.parent;
        if (setStatus) g_data->mStatus = mode.preheat ? STATUS_PREHEAT : STATUS_WORKING;
    }   break;
    }
    return isTrue;
}

void TuyaMgr::checkTuyaReserveReset() {
    if (g_data->mStatus != STATUS_RESERVE) return;

    // 当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);

    uint8_t hour = mReserveTimeReset / 60;
    uint8_t minute = mReserveTimeReset % 60;
    bool today = (hour > now_tm.tm_hour || (hour == now_tm.tm_hour && minute > now_tm.tm_min));

    // 计算结束时间并构建为时间戳
    std::tm target_tm = now_tm;
    target_tm.tm_mday += !today;
    target_tm.tm_hour = hour;
    target_tm.tm_min = minute;
    target_tm.tm_sec = 0;

    // 构建开始工作时间
    std::time_t target_time = std::mktime(&target_tm);
    target_tm.tm_hour -= g_data->mWorkAllTime / 3600;
    target_tm.tm_min -= (g_data->mWorkAllTime % 3600) / 60;
    target_time = std::mktime(&target_tm);

    LOGE("target_time: %s", std::ctime(&target_time));

    g_data->setBuzzer(BUZZER_DEFAULT, 1);
    g_data->mReserveTime = std::move(*std::localtime(&target_time));
    g_windMgr->goTo(PAGE_RESERVE);
}

void TuyaMgr::dealOTAComm(uint8_t* data, uint16_t len) {
    if (mOTALen != 0) {
        mOTALen = 0;
        system(RM_OTA_PATH);
    }

    mOTALen = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    mOTAAcceptTime = SystemClock::uptimeMillis(); // 记录接收数据的时间
    uint8_t send[1] = { OTA_PACKAGE_LEVEL };
    send2MCU(send, 1, TYCOMM_OTA_START);

    LOGI("[OTA START] allLen=%d oneByte=%d", mOTALen, send[0]);
    g_windMgr->goTo(PAGE_OTA);
}

void TuyaMgr::dealOTAData(uint8_t* data, uint16_t len) {
    uint32_t dataLen = len - 4;
    uint32_t dataOffSet = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    if (!dataLen || dataOffSet >= mOTALen) { // 数据传输完成
        system("/customer/upgrade.sh");
        system("reboot");
    } else {
        FILE* fp = fopen(OTA_SAVE_PATH, "ab");
        if (fp) {
            size_t ret = fwrite(data + 4, 1, dataLen, fp);
            if (ret != dataLen) LOGE("write file error");
            fclose(fp);
        } else {
            LOGE("Failed to open file for writing: %s", OTA_SAVE_PATH);
            // system("reboot");
        }
        LOGW("[OTA PROGRESS] %d/%d", dataOffSet + dataLen, mOTALen);
    }
    mOTAAcceptTime = SystemClock::uptimeMillis(); // 记录接收数据的时间
    mOTACurLen = dataOffSet + dataLen;
    send2MCU(TYCOMM_OTA_DATA);
}
