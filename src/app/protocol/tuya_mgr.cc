/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-08-01 03:03:02
 * @LastEditTime: 2026-07-14 23:22:14
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

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "app_version.h"

#include "wind_mgr.h"
#include "string_utils.h"
#include "system_utils.h"

#define TICK_TIME 50 // tick触发时间（毫秒）

//////////////////////////////////////////////////////////////////

typedef PacketBufferPoolT<BT_TUYA, TuyaAsk, TuyaAck> TuyaPacketBufferPool;

namespace {

constexpr uint16_t MAX_PAYLOAD_LEN =
    TuyaAck::BUFFER_CAPACITY - TuyaAck::FRAME_OVERHEAD;

struct CommandPayloadRule {
    uint8_t  command;
    uint16_t minLength;
    uint16_t maxLength;
};

// 所有当前处理的模组上行命令都必须先经过此表，再进入业务解析器。
const CommandPayloadRule COMMAND_PAYLOAD_RULES[] = {
    { TYCOMM_HEART,        0, 1 },
    { TYCOMM_INFO,         0, 0 },
    { TYCOMM_CHECK_MDOE,   0, 0 },
    { TYCOMM_WIFI_STATUS,  1, 1 },
    { TYCOMM_ACCEPT,       5, MAX_PAYLOAD_LEN },
    { TYCOMM_CHECK,        0, 0 },
    { TYCOMM_WIFITEST,     2, 2 },
    { TYCOMM_OTA_START,    4, 4 },
    { TYCOMM_OTA_DATA,     4, MAX_PAYLOAD_LEN },
    { TYCOMM_GET_TIME,     7, 7 },
    { TYCOMM_OPEN_WEATHER, 1, 2 },
    { TYCOMM_WEATHER,      1, MAX_PAYLOAD_LEN },
    { TYCOMM_OPEN_TIME,    2, 8 },
};

bool validatePayloadLength(uint8_t command, uint16_t length) {
    for (const CommandPayloadRule& rule : COMMAND_PAYLOAD_RULES) {
        if (rule.command != command) continue;
        if (length >= rule.minLength && length <= rule.maxLength) return true;
        LOGE("Reject Tuya command payload. cmd=0x%02x len=%u expected=%u..%u",
            command, length, rule.minLength, rule.maxLength);
        return false;
    }
    return true;
}

struct DpField {
    uint8_t        id;
    uint8_t        type;
    const uint8_t* value;
    uint16_t       length;
};

struct WeatherField {
    std::string name;
    bool        isString;
    std::string stringValue;
    int32_t     integerValue;
};

} // namespace

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
    mInitialized = true;

    // 启动延迟一会后开始发包
    setTick(TICK_TIME);
    startTick(TICK_TIME * 10);

    return 0;
}

void TuyaMgr::onTick(int64_t nowMs) {
    if (mIsRunConnectWork) {
        if (nowMs - mLastSendDiffDPTime >= 400) {
            if (sendDiffDp()) mLastSendDiffDPTime = nowMs;
        }

        if (nowMs - mLastSyncDateTime >= 1080000) {
            getTuyaTime();
        }
    } else {
        if (mNetWorkDetail == 0x04 &&
            nowMs - mNetWorkConnectTime >= 1000) {
            const bool weatherOpened = openWeatherServe();
            // send2MCU(TYCOMM_GET_TIME);
            const bool timeOpened = openTimeServe();
            mIsRunConnectWork = weatherOpened && timeOpened;
        }
    }
}

bool TuyaMgr::send2MCU(uint8_t cmd) {
    return send2MCU(nullptr, 0, cmd);
}

bool TuyaMgr::send2MCU(const uint8_t* buf, size_t len, uint8_t cmd) {
    if (!mInitialized || mPacket == nullptr || mChannel == nullptr) {
        LOGE("TuyaMgr send rejected before successful init. cmd=0x%02x", cmd);
        return false;
    }
    if (len > MAX_PAYLOAD_LEN || (len > 0 && buf == nullptr)) {
        LOGE("TuyaMgr invalid send payload. cmd=0x%02x len=%zu max=%u data=%p",
            cmd, len, MAX_PAYLOAD_LEN, static_cast<const void*>(buf));
        return false;
    }

    const uint16_t payloadLen = static_cast<uint16_t>(len);
    BuffData* bd = mPacket->obtainSend(payloadLen);
    if (bd == nullptr) {
        LOGE("TuyaMgr packet allocation failed. data_len=%u cmd=%u", payloadLen, cmd);
        return false;
    }
    TuyaAsk   snd(bd);

    snd.setData(TUYA_VERSION, 0x03);
    snd.setData(TUYA_COMM, cmd);
    snd.setData(TUYA_DATA_LEN_H, (payloadLen >> 8) & 0xFF);
    snd.setData(TUYA_DATA_LEN_L, payloadLen & 0xFF);
    snd.setData(buf, TUYA_DATA_START, payloadLen);

    snd.checkCode();    // 修改检验位
    LOG(VERBOSE) << "send to tuya. bytes=" << StringUtils::hexStr(bd->buf, bd->len);
    if (mChannel->send(bd) != 0) {
        LOGE("TuyaMgr send failed. cmd=0x%02x len=%u", cmd, payloadLen);
        return false;
    }
    mLastSendTime = cdroid::SystemClock::uptimeMillis();
    return true;
}

void TuyaMgr::onCommDeal(const IAck* ack) {
    if (!mInitialized || ack == nullptr) {
        LOGE("Reject Tuya packet while manager is unavailable");
        return;
    }
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
    if (!validatePayloadLength(command, payloadLen)) return;

    bool show = false;
    bool handled = true;
    switch (command) {
    case TYCOMM_HEART:
        handled = sendHeartBeat();
        break;
    case TYCOMM_INFO:
        if (mClearWifi) {
            handled = sendSetConnectMode(1);
        } else {
            handled = sendSetConnectMode(2);
        }
        break;
    case TYCOMM_CHECK_MDOE:
        handled = sendSetWorkMode();
        break;
    case TYCOMM_WIFI_STATUS:
        handled = sendWIFIStatus(payload[0]);
        break;
    case TYCOMM_ACCEPT:
        handled = acceptDP(payload, payloadLen);
        break;
    case TYCOMM_CHECK:
        handled = sendDp();
        break;
    case TYCOMM_WIFITEST:
        mWifiTestRes = static_cast<uint16_t>(payload[0])
            | (static_cast<uint16_t>(payload[1]) << 8);
        break;

    case TYCOMM_GET_TIME:
        handled = acceptTime(payload, payloadLen);
        break;
    case TYCOMM_OPEN_WEATHER:
        handled = acceptOpenWeather(payload, payloadLen);
        break;
    case TYCOMM_WEATHER:
        handled = acceptWeather(payload, payloadLen);
        break;
    case TYCOMM_OPEN_TIME:
        handled = acceptOpenTime(payload, payloadLen);
        break;

    case TYCOMM_OTA_START:
    case TYCOMM_OTA_DATA:
        rejectOTA(command, payloadLen);
        handled = false;
        break;
    default:
        show = true;
        LOG(INFO) << "[default]accept. bytes=" << StringUtils::hexStr(ack->data(), ack->dataLength());
        break;
    }

    if (!handled) {
        LOGE("Tuya command was not completed. cmd=0x%02x len=%u", command, payloadLen);
    }

    if (!show)
        LOG(VERBOSE) << "[default]accept. bytes=" << StringUtils::hexStr(ack->data(), ack->dataLength());

    mLastAcceptTime = cdroid::SystemClock::uptimeMillis();
}

bool TuyaMgr::sendHeartBeat() {
    LOG(VERBOSE) << "心跳";
    static bool firstSendHeartBeat = false;
    uint8_t data[1] = { 0x01 };
    if (!firstSendHeartBeat) {
        data[0] = 0x00;
    }
    const bool sent = send2MCU(data, sizeof(data), TYCOMM_HEART);
    if (sent) firstSendHeartBeat = true;
    return sent;
}

bool TuyaMgr::sendWifiTest() {
    LOG(VERBOSE) << "WIFI测试";
    if (!send2MCU(TYCOMM_WIFITEST)) return false;
    mWifiTestRes = 0xFFFF; // 复位
    return true;
}

bool TuyaMgr::resetWifi(bool clear) {
    LOG(VERBOSE) << "重置wifi" << (clear ? "[清空]" : "[非清空]");
    uint8_t data[2] = { 0x00,0x00 };
    if (!send2MCU(data, sizeof(data), TYCOMM_RESET)) return false;
    mClearWifi = clear;
    mNetWorkDetail = 0;
    mNetWork = WIFI_NULL;
    return true;
}

bool TuyaMgr::sendSetConnectMode(uint8_t mode) {
    LOG(VERBOSE) << "设置配网模式";

    std::string version{ APP_VERSION };
    int count = 0;
    for (char c : version)if (c == '.')count++;
    FailFast(count != 2,
        "APP_VERSION 格式错误，涂鸦版本号必须为 x.x.x，当前值：%s",
        version.c_str());

    std::string str = "{\"p\":\"cdwan0nqtmbyqvx3\",\"v\":\"" + version + "\",\"m\":" + std::to_string(mode % 10) + "}";
    LOGE("[tuyaConfig] -> %s", str.c_str());
    std::array<uint8_t, 0x2a> data{{ 0 }};
    if (str.size() > data.size()) {
        LOGE("Tuya config payload is too long. len=%zu max=%zu", str.size(), data.size());
        return false;
    }
    memcpy(data.data(), str.data(), str.size());
    return send2MCU(data.data(), data.size(), TYCOMM_INFO);
}

bool TuyaMgr::sendSetWorkMode() {
    return send2MCU(TYCOMM_CHECK_MDOE);
}

bool TuyaMgr::sendWIFIStatus(uint8_t status) {
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
    const bool sent = send2MCU(TYCOMM_WIFI_STATUS);

    if (mClearWifi) { resetWifi(); mClearWifi = false; }
    return sent;
}

bool TuyaMgr::getTuyaTime() {
    LOG(VERBOSE) << "获取涂鸦时间";
    if (!send2MCU(TYCOMM_GET_TIME)) return false;
    mLastSyncDateTime = cdroid::SystemClock::uptimeMillis();
    return true;
}

bool TuyaMgr::openTimeServe() {
    LOG(VERBOSE) << "开启涂鸦时间服务";
    uint8_t data[2] = { 0x01 ,0x01 };
    if (!send2MCU(data, sizeof(data), TYCOMM_OPEN_TIME)) return false;
    mLastSyncDateTime = cdroid::SystemClock::uptimeMillis();
    return true;
}

bool TuyaMgr::openWeatherServe() {
    LOG(VERBOSE) << "开启涂鸦天气服务";
    std::vector<std::string> strings = {
            "w.temp",
            "w.conditionNum",
            "w.thigh",
            "w.tlow",
            "w.date.1"
    };

    size_t totalLength = 0;
    for (const auto& str : strings) {
        if (str.size() > std::numeric_limits<uint8_t>::max()) {
            LOGE("Tuya weather key is too long. len=%zu", str.size());
            return false;
        }
        totalLength += str.length() + 1;
    }
    if (totalLength > std::numeric_limits<uint16_t>::max()) {
        LOGE("Tuya weather payload is too long. len=%zu", totalLength);
        return false;
    }

    std::vector<uint8_t> data;
    data.reserve(totalLength);

    for (const auto& str : strings) {
        data.push_back(static_cast<uint8_t>(str.size()));
        data.insert(data.end(), str.begin(), str.end());
    }
    return send2MCU(data.data(), data.size(), TYCOMM_OPEN_WEATHER);
}

bool TuyaMgr::sendDp() {
    LOG(VERBOSE) << "开机DP全上报";
    std::vector<uint8_t> payload;
    payload.reserve(32);

    const bool power = mPower;
    if (!appendDp(payload, TYDPID_POWER, TUYATYPE_BOOL, &power, 1)) {
        return false;
    }

    if (!send2MCU(payload.data(), payload.size(), TYCOMM_SEND)) return false;
    mCachePower = power;
    mLastSendDiffDPTime = cdroid::SystemClock::uptimeMillis();
    return true;
}

bool TuyaMgr::sendDiffDp() {
    std::vector<uint8_t> payload;
    payload.reserve(32);

    if (mPower != mCachePower) {
        const bool power = mPower;
        if (!appendDp(payload, TYDPID_POWER, TUYATYPE_BOOL, &power, 1)) {
            return false;
        }
        if (!send2MCU(payload.data(), payload.size(), TYCOMM_SEND)) return false;
        mCachePower = power;
        LOG(VERBOSE) << "DP差异上报";
        return true;
    }

    return true;
}

uint8_t TuyaMgr::getOTAProgress() {
    // 可信镜像校验链路完成前，远程OTA保持禁用。
    return 0;
}

uint64_t TuyaMgr::getOTAAcceptTime() {
    return mOTAAcceptTime;
}

bool TuyaMgr::appendDp(std::vector<uint8_t>& payload, uint8_t dp, uint8_t type,
    const void* data, size_t dlen, bool reverse) {
    if (data == nullptr || dlen == 0 || dlen > std::numeric_limits<uint16_t>::max()) {
        LOGE("Invalid Tuya DP value. id=%u len=%zu data=%p",
            dp, dlen, data);
        return false;
    }
    const size_t maxPayload = std::numeric_limits<uint16_t>::max();
    if (dlen > maxPayload - 4 || payload.size() > maxPayload - 4 - dlen) {
        LOGE("Tuya DP payload exceeds protocol limit. current=%zu add=%zu",
            payload.size(), dlen + 4);
        return false;
    }

    const uint16_t valueLength = static_cast<uint16_t>(dlen);
    payload.push_back(dp);
    payload.push_back(type);
    payload.push_back(static_cast<uint8_t>((valueLength >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(valueLength & 0xFF));

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    if (reverse) {
        for (size_t i = 0; i < dlen; ++i) payload.push_back(bytes[dlen - 1 - i]);
    } else {
        payload.insert(payload.end(), bytes, bytes + dlen);
    }
    return true;
}

bool TuyaMgr::acceptDP(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len < 5) {
        LOGE("Reject short Tuya DP payload. len=%u", len);
        return false;
    }

    std::vector<DpField> fields;
    size_t cursor = 0;
    while (cursor < len) {
        if (static_cast<size_t>(len) - cursor < 4) {
            LOGE("Reject truncated Tuya DP header. cursor=%zu len=%u", cursor, len);
            return false;
        }

        DpField field;
        field.id = data[cursor];
        field.type = data[cursor + 1];
        field.length = (static_cast<uint16_t>(data[cursor + 2]) << 8)
            | static_cast<uint16_t>(data[cursor + 3]);
        cursor += 4;
        if (field.length == 0 || field.length > static_cast<size_t>(len) - cursor) {
            LOGE("Reject invalid Tuya DP length. id=%u value_len=%u remaining=%zu",
                field.id, field.length, static_cast<size_t>(len) - cursor);
            return false;
        }
        field.value = data + cursor;
        cursor += field.length;

        if (field.id == TYDPID_POWER
            && (field.type != TUYATYPE_BOOL || field.length != 1 || field.value[0] > 1)) {
            LOGE("Reject invalid power DP. type=%u len=%u value=%u",
                field.type, field.length, field.value[0]);
            return false;
        }
        fields.push_back(field);
    }

    LOG(WARN) << "accept dp[" << StringUtils::fill(fields.front().id, 3)
        << "]. bytes=" << StringUtils::hexStr(data, len);
    MainWindow* window = g_windMgr->getWindow();
    for (const DpField& field : fields) {
        if (field.id == TYDPID_POWER && window == nullptr) {
            LOGE("Reject power DP before window initialization");
            return false;
        }
    }
    for (const DpField& field : fields) {
        // 关机状态下，仅处理开机指令。
        if (!mPower && field.id != TYDPID_POWER) continue;

        switch (field.id) {
        case TYDPID_POWER:
            if (field.value[0]) {
                window->hideBlack();
            } else {
                g_windMgr->goToHome(false);
                window->showBlack();
            }
            mPower = field.value[0] != 0;
            break;
        default:
            break;
        }
    }

    LOGI("final Deal Tuya Dp");
    return true;
}

bool TuyaMgr::acceptTime(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len != 7) {
        LOGE("Reject invalid Tuya time payload. len=%u", len);
        return false;
    }
    if (!data[0]) return true; // 模组返回失败状态，不读取后续业务字段
    SystemUtils::setTime(data[1] + 2000, data[2], data[3], data[4], data[5], data[6]);
    return true;
}

bool TuyaMgr::acceptOpenWeather(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len < 1 || len > 2 || (!data[0] && len != 2)) {
        LOGE("Reject invalid Tuya weather-open payload. len=%u", len);
        return false;
    }
    if (data[0])LOGI("Weather Serve Open Success");
    else LOGE("Weather Serve Open Failed!!!  -> code: %d", data[1]);
    return true;
}

bool TuyaMgr::acceptWeather(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len < 1) {
        LOGE("Reject empty Tuya weather payload");
        return false;
    }
    LOG(VERBOSE) << "accept weather. bytes=" << StringUtils::hexStr(data, len);

    std::vector<WeatherField> fields;
    size_t cursor = 1; // 首字节为天气服务状态/版本字段
    while (cursor < len) {
        const uint8_t nameLength = data[cursor++];
        if (nameLength == 0 || nameLength > static_cast<size_t>(len) - cursor) {
            LOGE("Reject invalid Tuya weather name. name_len=%u remaining=%zu",
                nameLength, static_cast<size_t>(len) - cursor);
            return false;
        }

        WeatherField field;
        field.name.assign(reinterpret_cast<const char*>(data + cursor), nameLength);
        field.isString = false;
        field.integerValue = 0;
        cursor += nameLength;

        if (static_cast<size_t>(len) - cursor < 2) {
            LOGE("Reject truncated Tuya weather value header. name=%s", field.name.c_str());
            return false;
        }
        const uint8_t valueType = data[cursor++];
        const uint8_t valueLength = data[cursor++];
        if (valueLength == 0 || valueLength > static_cast<size_t>(len) - cursor) {
            LOGE("Reject invalid Tuya weather value length. name=%s len=%u remaining=%zu",
                field.name.c_str(), valueLength, static_cast<size_t>(len) - cursor);
            return false;
        }

        if (valueType == 0) {
            if (valueLength > sizeof(uint32_t)) {
                LOGE("Reject oversized Tuya weather integer. name=%s len=%u",
                    field.name.c_str(), valueLength);
                return false;
            }
            uint32_t rawValue = 0;
            for (uint8_t i = 0; i < valueLength; ++i) {
                rawValue = (rawValue << 8) | data[cursor + i];
            }
            int64_t signedValue = rawValue;
            if ((data[cursor] & 0x80) != 0) {
                signedValue -= static_cast<int64_t>(uint64_t{ 1 } << (valueLength * 8));
            }
            field.integerValue = static_cast<int32_t>(signedValue);
        } else if (valueType == 1) {
            field.isString = true;
            field.stringValue.assign(
                reinterpret_cast<const char*>(data + cursor), valueLength);
        } else {
            LOGE("Reject unknown Tuya weather value type. name=%s type=%u",
                field.name.c_str(), valueType);
            return false;
        }
        cursor += valueLength;
        fields.push_back(field);
    }

    for (const WeatherField& field : fields) {
        const bool isTemperature = field.name == "w.temp" || field.name == "w.temp.0"
            || field.name == "w.thigh" || field.name == "w.thigh.0"
            || field.name == "w.tlow" || field.name == "w.tlow.0";
        if (isTemperature && (field.isString
            || field.integerValue < std::numeric_limits<int8_t>::min()
            || field.integerValue > std::numeric_limits<int8_t>::max())) {
            LOGE("Reject invalid Tuya weather temperature. name=%s value=%d",
                field.name.c_str(), field.integerValue);
            return false;
        }
        const bool isCondition = field.name == "w.conditionNum"
            || field.name == "w.conditionNum.0";
        if (isCondition && !field.isString) {
            LOGE("Reject non-string Tuya weather condition");
            return false;
        }
    }

    // 全部TLV通过结构和语义校验后再一次性发布，避免畸形尾项造成部分更新。
    for (const WeatherField& field : fields) {
        LOGI("name: %s value: %s int: %d", field.name.c_str(),
            field.stringValue.c_str(), field.integerValue);
        if (field.name == "w.temp" || field.name == "w.temp.0") {
            mTem = static_cast<int8_t>(field.integerValue);
        } else if (field.name == "w.conditionNum" || field.name == "w.conditionNum.0") {
            mWeather = field.stringValue;
        } else if (field.name == "w.thigh" || field.name == "w.thigh.0") {
            mTemMax = static_cast<int8_t>(field.integerValue);
        } else if (field.name == "w.tlow" || field.name == "w.tlow.0") {
            mTemMin = static_cast<int8_t>(field.integerValue);
        }
    }
    return send2MCU(TYCOMM_WEATHER);
}

bool TuyaMgr::acceptOpenTime(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len < 2) {
        LOGE("Reject short Tuya time-service payload. len=%u", len);
        return false;
    }
    switch (data[0]) {
    case 0x01:
        if (len != 2) {
            LOGE("Reject Tuya time-service status length. len=%u", len);
            return false;
        }
        return !data[1] || getTuyaTime();
    case 0x02:
        if (len != 8) {
            LOGE("Reject Tuya time-service push length. len=%u", len);
            return false;
        }
        if (!data[1]) return true;
        SystemUtils::setTime(data[2] + 2000, data[3], data[4], data[5], data[6], data[7]);
        return true;
    default:
        LOGE("Reject unknown Tuya time-service subtype. type=%u", data[0]);
        return false;
    }
}

void TuyaMgr::rejectOTA(uint8_t command, uint16_t len) {
    mOTAAcceptTime = cdroid::SystemClock::uptimeMillis();
    LOGE("Reject remote Tuya OTA command. cmd=0x%02x len=%u reason="
        "trusted digest/signature verification and OTA UI are not configured",
        command, len);
}
