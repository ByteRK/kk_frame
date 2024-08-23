#ifndef __proto_h__
#define __proto_h__
/**
 * 交互协议相关定义
*/

#include "common.h"


////////////////////////////////////////////////////////////////////////////////////////////////
// 数据发送的结构

enum {
    S_BUF_HEAD = 0,      // 帧头
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 数据接收的结构

enum {
    R_BUF_HEAD = 0,      // 帧头
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 工作状态

enum {
    STATUS_STANDBY = 0,    // 待机
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 涂鸦

// 消息结构
enum {
    TUYA_HEAD1 = 0,        // 帧头
    TUYA_HEAD2,            // 帧头
    TUYA_VERSION,          // 版本
    TUYA_COMM,             // 命令字
    TUYA_DATA_LEN_H,       // 数据长度高位
    TUYA_DATA_LEN_L,       // 数据长度低位
    TUYA_DATA_START,       // 数据起始位置
};

// 涂鸦数据类型
enum {
    TUYATYPE_RAW = 0x00,     // RAW数据(透传)
    TUYATYPE_BOOL = 0x01,    // 布尔型数据
    TUYATYPE_VALUE = 0x02,   // 数值型数据(大端)
    TUYATYPE_STRING = 0x03,  // 字符串型数据
    TUYATYPE_ENUM = 0x04,    // 枚举型数据
    TUYATYPE_BITMAP = 0x05,  // 位图型数据(大端)
};

// DP数据结构
enum {
    TUYADP_ID = 0,        // DP ID
    TUYADP_TYPE,          // 数据类型
    TUYADP_LEN_H,         // 数据长度高位
    TUYADP_LEN_L,         // 数据长度低位
    TUYADP_DATA,          // 数据起始位置
};

// 命令
enum {
    TYCOMM_HEART = 0,            // <-> 心跳检测
    TYCOMM_INFO,                 // <-> 查询产品信息
    TYCOMM_CHECK_MDOE,           // <-> 查询 MCU 设定模块工作方式
    TYCOMM_WIFI_STATUS,          // <-> 报告 WiFi 工作状态
    TYCOMM_RESET,                // --> 重置 WiFi
    TYCOMM_RESET_MODE,           // --> 重置 WiFi 工作方式
    TYCOMM_ACCEPT = 0x06,        // <-- 模组命令下发
    TYCOMM_SEND = 0x07,          // --> 状态上报（异步）
    TYCOMM_CHECK = 0x08,         // <-- 模组状态查询
    TYCOMM_SEND_SYNC = 0x22,     // --> 状态上报（同步）

    TYCOMM_OTA_START = 0x0A,     // <-> 开始OTA
    TYCOMM_OTA_DATA = 0x0B,      // <-> OTA数据

    TYCOMM_GET_TIME = 0x1c,      // <-> 获取时间
    TYCOMM_OPEN_WEATHER = 0x20,  // --> 开启天气服务
    TYCOMM_WEATHER = 0x21,       // <-> 天气数据
    TYCOMM_OPEN_TIME = 0x34,     // <-> 开启时间服务
};

// 涂鸦DP ID枚举
enum {
    TYDPID_SWITCH = 0x01,       // 开关
};


#endif
