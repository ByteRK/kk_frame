/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-01 03:03:02
 * @LastEditTime: 2025-11-26 17:47:34
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
#include "app_version.h"

#include "conn_mgr.h"
#include "wind_mgr.h"
#include "this_func.h"
#include "global_data.h"

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
    mPacket = new TuyaPacketBuffer();
    mUartTUYA = 0;
    mNextEventTime = 0;
    mLastSendTime = 0;
    mLastSyncDateTime = 0;
    mLastSendDiffDPTime = 0;

    g_packetMgr->addHandler(BT_TUYA, this);
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

    mUartTUYA = new UartClient(mPacket, ss, "192.168.0.113", 1144, 0, true);
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
    BuffData* bd = mPacket->obtain(false, len);
    TuyaAsk   snd(bd);

    snd.setData(TUYA_VERSION, 0x03);
    snd.setData(TUYA_COMM, cmd);
    snd.setData(TUYA_DATA_LEN_H, (len >> 8) & 0xFF);
    snd.setData(TUYA_DATA_LEN_L, len & 0xFF);
    snd.setData(buf, TUYA_DATA_START, len);

    snd.checkCode();    // 修改检验位
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
    }   break;
    case TYCOMM_CHECK:
        sendDp();
        break;
    case TYCOMM_WIFITEST:
        g_data->mTUYAWifiTestRes = ack->getData2(TUYA_DATA_START, true);
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
        LOG(INFO) << "[default]accept. bytes=" << hexstr(ack->mBuf, ack->mDataLen);
        break;
    }

    if (!show)
        LOG(VERBOSE) << "[default]accept. bytes=" << hexstr(ack->mBuf, ack->mDataLen);

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
    g_data->mTUYAWifiTestRes = 0xFFFF; // 复位
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

    std::string version{ APP_VERSION };
    int count = 0;
    for (char c : version)if (c == '.')count++;
    if (count != 2)throw std::runtime_error("APP_VERSION 格式错误!!! 涂鸦版本号必须为x.x.x");

    std::string str = "{\"p\":\"cdwan0nqtmbyqvx3\",\"v\":\"" + version + "\",\"m\":" + std::to_string(mode % 10) + "}";
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
    buf[count + 0] = dp;                     // DP ID
    buf[count + 1] = type;                   // 类型
    buf[count + 2] = (dlen >> 8) & 0xFF;     // 数据长度高位
    buf[count + 3] = dlen & 0xFF;            // 数据长度低位

    if (reverse) { // 逆序复制 ui8Data 到 buf + count + 4
        for (int i = 0; i < dlen; ++i) buf[count + 4 + i] = ui8Data[dlen - 1 - i];
    } else { // 正序复制 ui8Data 到 buf + count + 4
        memcpy(buf + count + 4, ui8Data, dlen);
    }
    count += (4 + dlen); // 累加计数
}

void TuyaMgr::acceptDP(uint8_t* data, uint16_t len) {
    LOGE("len = %d", len);
    LOG(WARN) << "accept dp[" << fillLength(data[0], 3) << "]. bytes=" << hexstr(data, len);
    uint16_t dealCount = 0;

    do {
        // 关机状态下，仅处理开机指令
        if (!g_data->mTUYAPower && data[dealCount] != TYDPID_POWER) {
            break;
        }

        switch (data[dealCount]) {
        case TYDPID_POWER: {
            if (data[TUYADP_DATA]) {
                g_window->hideBlack();
            } else {
                g_windMgr->showPage(PAGE_HOME);
                g_window->showBlack();
            }
            g_data->mTUYAPower = data[TUYADP_DATA];
        }   break;
        default:
            break;
        }
        dealCount += (4 + ((uint16_t)data[dealCount + TUYADP_LEN_H] << 8 | data[dealCount + TUYA_DATA_LEN_L]));
    } while (false && len - dealCount > 0);

    LOGI("final Deal Tuya Dp");
}

void TuyaMgr::acceptTime(uint8_t* data) {
    if (!data[0])return; // 消息错误
    timeSet(data[1] + 2000, data[2], data[3], data[4], data[5], data[6]);
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
    g_windMgr->showPage(PAGE_OTA);
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
