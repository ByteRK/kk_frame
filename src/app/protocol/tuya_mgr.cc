/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-01 03:03:02
 * @LastEditTime: 2026-07-14 17:04:01
 * @FilePath: /kk_frame/src/app/protocol/tuya_mgr.cc
 * @Description: 涂鸦模组通讯
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "tuya_mgr.h"
#include "tuya_packet.h"
#include "project_utils.h"
#include "app_common.h"
#include <core/app.h>
#include <core/systemclock.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "config_mgr.h"
#include "app_version.h"

#include "mcu_mgr.h"
#include "wind_mgr.h"
#include "string_utils.h"
#include "system_utils.h"

#define TICK_TIME 50 // tick触发时间（毫秒）

/**
 * 分包大小
 * 0x00：默认 256 bytes（兼容旧固件）
 * 0x01：512 bytes
 * 0x02：1024 bytes
**/
#define OTA_PACKAGE_LEVEL 0x02

#define OTA_SAVE_PATH  "/tmp/kaidu_t2e_pro.tar.gz"
#define RM_OTA_PATH  "rm -rf " OTA_SAVE_PATH

//////////////////////////////////////////////////////////////////

typedef PacketBufferPoolT<BT_TUYA, TuyaAsk, TuyaAck> TuyaPacketBufferPool;

TuyaMgr::TuyaMgr() {
    mPacket = new TuyaPacketBufferPool();
}

TuyaMgr::~TuyaMgr() {
    mInitialized = false;
    stopTick();
    __delete(mChannel);
    __delete(mPacket);
}

int TuyaMgr::init() {
    if (mInitialized) return 0;
    LOGI("开始监听");

#ifdef PRODUCT_X64
    TcpClient::Config config;
    ProjectUtils::getDebugServiceInfo(config.host, config.port);
    config.port += BT_TUYA;
#else
    UartClient::Config config;
    config.device = "/dev/ttyS3";
    config.baudRate = 9600;
    config.flowControl = 0;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = 'N';
    config.pollIntervalMs = 200;
#endif

    TuyaCommChannel* channel = new TuyaCommChannel(mPacket, true, config);
    const int rc = channel->init();
    if (rc != 0) {
        LOGE("TuyaMgr channel init failed. rc=%d", rc);
        delete channel;
        return rc;
    }
    if (!g_packetMgr->addHandler(BT_TUYA, this)) {
        LOGE("TuyaMgr packet handler registration failed");
        delete channel;
        return -4;
    }
    mChannel = channel;

    // 启动延迟一会后开始发包
    setTick(TICK_TIME);
    startTick(TICK_TIME * 10);

    mInitialized = true;
    return 0;
}

void TuyaMgr::onTick(int64_t nowMs) {
    if (mIsRunConnectWork) {
        if (nowMs - mLastSendDiffDPTime >= 400) {
            sendDiffDp();
            mLastSendDiffDPTime = nowMs;
        }

        if (nowMs - mLastSyncDateTime >= 1080000) {
            getTuyaTime();
        }
    } else {
        if (mNetWorkDetail == 0x04 &&
            nowMs - mNetWorkConnectTime >= 1000) {
            openWeatherServe();
            // send2MCU(TYCOMM_GET_TIME);
            openTimeServe();
            mIsRunConnectWork = true;
        }
    }
}

void TuyaMgr::send2MCU(uint8_t cmd) {
    send2MCU(0, 0, cmd);
}

void TuyaMgr::send2MCU(uint8_t* buf, uint16_t len, uint8_t cmd) {
    BuffData* bd = mPacket->obtainSend(len);
    if (bd == nullptr) {
        LOGE("TuyaMgr packet allocation failed. data_len=%u cmd=%u", len, cmd);
        return;
    }
    TuyaAsk   snd(bd);

    snd.setData(TUYA_VERSION, 0x03);
    snd.setData(TUYA_COMM, cmd);
    snd.setData(TUYA_DATA_LEN_H, (len >> 8) & 0xFF);
    snd.setData(TUYA_DATA_LEN_L, len & 0xFF);
    snd.setData(buf, TUYA_DATA_START, len);

    snd.checkCode();    // 修改检验位
    LOG(VERBOSE) << "send to tuya. bytes=" << StringUtils::hexStr(bd->buf, bd->len);
    mChannel->send(bd);
    mLastSendTime = cdroid::SystemClock::uptimeMillis();
}

void TuyaMgr::onCommDeal(const IAck* ack) {
    if (mLastAcceptTime == 0)mLastAcceptTime = cdroid::SystemClock::uptimeMillis();

    uint8_t command = 0;
    uint16_t payloadLen = 0;
    const uint8_t* payload = nullptr;
    if (!ack->readU8(TUYA_COMM, command)
        || !ack->readU16(TUYA_DATA_LEN_H, payloadLen)
        || !ack->readBytes(TUYA_DATA_START, payloadLen, payload)) {
        LOGE("Invalid Tuya packet fields. len=%u", ack->dataLength());
        return;
    }

    bool show = false;
    switch (command) {
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
    case TYCOMM_WIFI_STATUS: {
        uint8_t status = 0;
        if (!ack->readU8(TUYA_DATA_START, status)) {
            LOGE("Invalid Tuya WiFi status packet");
            break;
        }
        sendWIFIStatus(status);
    }   break;
    case TYCOMM_ACCEPT:
        acceptDP(payload, payloadLen);
        break;
    case TYCOMM_CHECK:
        sendDp();
        break;
    case TYCOMM_WIFITEST: {
        uint16_t result = 0;
        if (!ack->readU16(TUYA_DATA_START, result, IAck::ByteOrder::LittleEndian)) {
            LOGE("Invalid Tuya WiFi test packet");
            break;
        }
        mWifiTestRes = result;
    }   break;

    case TYCOMM_GET_TIME:
        acceptTime(payload);
        break;
    case TYCOMM_OPEN_WEATHER:
        acceptOpenWeather(payload);
        break;
    case TYCOMM_WEATHER:
        acceptWeather(payload, payloadLen);
        break;
    case TYCOMM_OPEN_TIME:
        acceptOpenTime(payload);
        break;

    case TYCOMM_OTA_START:
        dealOTAComm(payload, payloadLen);
        break;
    case TYCOMM_OTA_DATA:
        dealOTAData(payload, payloadLen);
        break;
    default:
        show = true;
        LOG(INFO) << "[default]accept. bytes=" << StringUtils::hexStr(ack->data(), ack->dataLength());
        break;
    }

    if (!show)
        LOG(VERBOSE) << "[default]accept. bytes=" << StringUtils::hexStr(ack->data(), ack->dataLength());

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
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
    mWifiTestRes = 0xFFFF; // 复位
    send2MCU(TYCOMM_WIFITEST);
}

void TuyaMgr::resetWifi(bool clear) {
    LOG(VERBOSE) << "重置wifi" << clear ? "[清空]" : "[非清空]";
    mClearWifi = clear;
    mNetWorkDetail = 0;
    mNetWork = WIFI_NULL;
    uint8_t data[2] = { 0x00,0x00 };
    send2MCU(data, 2, TYCOMM_RESET);
}

void TuyaMgr::sendSetConnectMode(uint8_t mode) {
    LOG(VERBOSE) << "设置配网模式";

    std::string version{ APP_VERSION };
    int count = 0;
    for (char c : version)if (c == '.')count++;
    FailFast(count != 2,
        "APP_VERSION 格式错误，涂鸦版本号必须为 x.x.x，当前值：%s",
        version.c_str());

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
    mNetWorkDetail = status;
    switch (status) {
    case 0x04:
        if (mNetWork != 0x04) {
            mIsRunConnectWork = false;
            mNetWorkConnectTime = cdroid::SystemClock::uptimeMillis();
        }
        mNetWork = WIFI_4;
        break;
    case 0x02:
        mNetWork = WIFI_ERROR;
        break;
    case 0x00:
    case 0x01:
    default:
        mNetWork = WIFI_NULL;
        break;
    }
    send2MCU(TYCOMM_WIFI_STATUS);

    if (mClearWifi) { resetWifi(); mClearWifi = false; }
}

void TuyaMgr::getTuyaTime() {
    LOG(VERBOSE) << "获取涂鸦时间";
    mLastSyncDateTime = cdroid::SystemClock::uptimeMillis();
    send2MCU(TYCOMM_GET_TIME);
}

void TuyaMgr::openTimeServe() {
    LOG(VERBOSE) << "开启涂鸦时间服务";
    uint8_t data[2] = { 0x01 ,0x01 };
    mLastSyncDateTime = cdroid::SystemClock::uptimeMillis();
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

    createDp(s_SendDpBuf, count, TYDPID_POWER, TUYATYPE_BOOL, &mCachePower, 1);

    mLastSendDiffDPTime = cdroid::SystemClock::uptimeMillis();
    send2MCU(s_SendDpBuf, count, TYCOMM_SEND);
}

void TuyaMgr::sendDiffDp() {
    static uint8_t s_SendDiffDpBuf[256];
    memset(s_SendDiffDpBuf, 0, 256);
    uint16_t count = 0;

    if (mPower != mCachePower) {
        mCachePower = mPower;
        createDp(s_SendDiffDpBuf, count, TYDPID_POWER, TUYATYPE_BOOL, &mCachePower, 1);
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

void TuyaMgr::acceptDP(const uint8_t* data, uint16_t len) {
    LOGE("len = %d", len);
    LOG(WARN) << "accept dp[" << StringUtils::fill(data[0], 3) << "]. bytes=" << StringUtils::hexStr(data, len);
    uint16_t dealCount = 0;

    do {
        // 关机状态下，仅处理开机指令
        if (!mPower && data[dealCount] != TYDPID_POWER) {
            break;
        }

        switch (data[dealCount]) {
        case TYDPID_POWER: {
            if (data[TUYADP_DATA]) {
                g_window->hideBlack();
            } else {
                g_windMgr->goToHome(false);
                g_window->showBlack();
            }
            mPower = data[TUYADP_DATA];
        }   break;
        default:
            break;
        }
        dealCount += (4 + ((uint16_t)data[dealCount + TUYADP_LEN_H] << 8 | data[dealCount + TUYA_DATA_LEN_L]));
    } while (false && len - dealCount > 0);

    LOGI("final Deal Tuya Dp");
}

void TuyaMgr::acceptTime(const uint8_t* data) {
    if (!data[0])return; // 消息错误
    SystemUtils::setTime(data[1] + 2000, data[2], data[3], data[4], data[5], data[6]);
}

void TuyaMgr::acceptOpenWeather(const uint8_t* data) {
    if (data[0])LOGI("Weather Serve Open Success");
    else LOGE("Weather Serve Open Failed!!!  -> code: %d", data[1]);
}

void TuyaMgr::acceptWeather(const uint8_t* data, uint16_t len) {
    LOG(VERBOSE) << "accept weather. bytes=" << StringUtils::hexStr(data, len);
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
                mTem = valueInt;
            } else if (name == "w.conditionNum" || name == "w.conditionNum.0") {
                mWeather = valueStr;
            } else if (name == "w.thigh" || name == "w.thigh.0") {
                mTemMax = valueInt;
            } else if (name == "w.tlow" || name == "w.tlow.0") {
                mTemMin = valueInt;
            }
        }
    }
    send2MCU(TYCOMM_WEATHER);
}

void TuyaMgr::acceptOpenTime(const uint8_t* data) {
    switch (data[0]) {
    case 0x01:
        if (data[1])getTuyaTime();
        break;
    case 0x02:
        if (!data[1])break;
        SystemUtils::setTime(data[2] + 2000, data[3], data[4], data[5], data[6], data[7]);
        break;
    }
}

void TuyaMgr::dealOTAComm(const uint8_t* data, uint16_t len) {
    if (mOTALen != 0) {
        mOTALen = 0;
        system(RM_OTA_PATH);
    }

    mOTALen = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    mOTAAcceptTime = cdroid::SystemClock::uptimeMillis(); // 记录接收数据的时间
    uint8_t send[1] = { OTA_PACKAGE_LEVEL };
    send2MCU(send, 1, TYCOMM_OTA_START);

    LOGI("[OTA START] allLen=%d oneByte=%d", mOTALen, send[0]);
    g_windMgr->showPage(PAGE_OTA);
}

void TuyaMgr::dealOTAData(const uint8_t* data, uint16_t len) {
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
    mOTAAcceptTime = cdroid::SystemClock::uptimeMillis(); // 记录接收数据的时间
    mOTACurLen = dataOffSet + dataLen;
    send2MCU(TYCOMM_OTA_DATA);
}
