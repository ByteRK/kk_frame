/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-01 03:03:02
 * @LastEditTime: 2024-08-23 09:40:16
 * @FilePath: /kk_frame/src/protocol/tuya_mgr.cc
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

#define TICK_TIME 100 // tick触发时间（毫秒）

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
        sendSetConnectMode(2);
        break;
    case TYCOMM_CHECK_MDOE:
        sendSetWorkMode();
        break;
    case TYCOMM_WIFI_STATUS:
        sendWIFIStatus(ack->getData(TUYA_DATA_START));
        break;
    case TYCOMM_ACCEPT:
        acceptDP(ack->mBuf + TUYA_DATA_START, ack->getData2(TUYA_DATA_LEN_H));
        break;
    case TYCOMM_CHECK:
        sendDp();
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

void TuyaMgr::resetWifi() {
    LOG(VERBOSE) << "重置wifi";
    g_data->mNetWorkDetail = 0;
    g_data->mNetWork = globalData::WIFI_NULL;
    uint8_t data[2] = { 0x00,0x00 };
    send2MCU(data, 2, TYCOMM_RESET);
}

void TuyaMgr::sendSetConnectMode(uint8_t mode) {
    LOG(VERBOSE) << "设置配网模式";
    std::string str = "{\"p\":\"bewmlg02cpi6vupq\",\"v\":\"" + std::string(HV_SOFT_VER_EXT) + "\",\"m\":" + std::to_string(mode % 10) + "}";
    LOGE("[tuyaConfig] -> %s", str.c_str());
    uint8_t data[0x2a];
    memcpy(data, str.c_str(), 0x2a);
    send2MCU(data, 0x2a, TYCOMM_INFO);
}

void TuyaMgr::sendSetWorkMode() {
    send2MCU(TYCOMM_CHECK_MDOE);
}

void TuyaMgr::sendWIFIStatus(uint8_t status) {
    LOG(VERBOSE) << "获取涂鸦时间";
    g_data->mNetWorkDetail = status;
    switch (status) {
    case 0x04:
        if (g_data->mNetWork != 0x04) {
            mIsRunConnectWork = false;
            mNetWorkConnectTime = SystemClock::uptimeMillis();
        }
        g_data->mNetWork = globalData::WIFI_4;
        break;
    case 0x02:
        g_data->mNetWork = globalData::WIFI_ERROR;
        break;
    case 0x00:
    case 0x01:
    default:
        g_data->mNetWork = globalData::WIFI_NULL;
        break;
    }
    send2MCU(TYCOMM_WIFI_STATUS);
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
    std::string str = "w.temp";
    std::string str2 = "w.conditionNum";
    uint8_t data[0x16];
    data[0] = 0x06;
    memcpy(data + 1, str.c_str(), 6);
    data[7] = 0x0E;
    memcpy(data + 8, str2.c_str(), 14);
    send2MCU(data, 22, TYCOMM_OPEN_WEATHER);
}

void TuyaMgr::sendDp() {
    LOG(VERBOSE) << "开机DP全上报";
    static uint8_t s_SendDpBuf[256];
    memset(s_SendDpBuf, 0, 256);
    uint16_t count = 0;

    // createDp(s_SendDpBuf, count, TYDPID_SWITCH, TUYATYPE_BOOL, &mSwitch, 1);

    mLastSendDiffDPTime = SystemClock::uptimeMillis();
    send2MCU(s_SendDpBuf, count, TYCOMM_SEND);
}

void TuyaMgr::sendDiffDp() {
    static uint8_t s_SendDiffDpBuf[256];
    memset(s_SendDiffDpBuf, 0, 256);
    uint16_t count = 0;

    // if (g_data->mSwitch != mSwitch) {
    //     mSwitch = g_data->mSwitch;
    //     createDp(s_SendDiffDpBuf, count, TYDPID_SWITCH, TUYATYPE_BOOL, &mSwitch, 1);
    // }

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

void TuyaMgr::acceptDP(uint8_t* data, uint16_t len) {
    LOG(WARN) << "accept dp. bytes=" << hexstr(data, len);
    uint16_t dealCount = 0;

    do {
        switch (data[dealCount]) {
        default:
            break;
        }
        dealCount += (4 + ((uint16_t)data[dealCount + TUYADP_LEN_H] << 8 | data[dealCount + TUYA_DATA_LEN_L]));
    } while (0 && len - dealCount > 0);
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
            dealCount += (2 + valuelen);

            if (name == "w.temp") {
                LOGI("Tem: %d", valueInt);
                // g_data->mTUYATem = valueInt;
            } else if (name == "w.conditionNum") {
                LOGI("Weather: %s", valueStr.c_str());
                // g_data->mTUYAWeather = valueStr;
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

void TuyaMgr::dealOTAComm(uint8_t* data, uint16_t len) {
    if (mOTALen != 0) {
        delete[] mOTAData;
        mOTAData = nullptr;
        mOTALen = 0;
    }

    mOTALen = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    mOTAData = new uint8_t[mOTALen];
    mOTAAcceptTime = SystemClock::uptimeMillis(); // 记录接收数据的时间
    uint8_t send[1] = { 0x01 };
    send2MCU(send, 1, TYCOMM_OTA_START);

    LOGI("[OTA START] allLen=%d oneByte=%d", mOTALen, send[0]);
    // g_windMgr->goTo(PAGE_OTA);
}

void TuyaMgr::dealOTAData(uint8_t* data, uint16_t len) {
    uint32_t dataLen = len - 4;
    uint32_t dataOffSet = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    if (!dataLen || dataOffSet >= mOTALen) { // 数据传输完成
        saveOTAData();
    } else {
        memcpy(mOTAData + dataOffSet, data + 4, dataLen);
        LOGW("[OTA PROGRESS] %d/%d", dataOffSet + dataLen, mOTALen);
    }
    mOTAAcceptTime = SystemClock::uptimeMillis(); // 记录接收数据的时间
    mOTACurLen = dataOffSet + dataLen;
    send2MCU(TYCOMM_OTA_DATA);
}

void TuyaMgr::saveOTAData() {
    if (mOTAData == nullptr || mOTALen == 0)return;
    std::ofstream outfile("/tmp/kk_frame.tar.gz", std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        LOGE("Error opening file for writing.");
        system("reboot");
    } else {
        outfile.write(reinterpret_cast<const char*>(mOTAData), mOTALen);
        outfile.close();
        system("/customer/upgrade.sh");
        system("reboot");
    }
    delete[] mOTAData;
    mOTAData = nullptr;
    mOTALen = 0;
}
