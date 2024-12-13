#ifndef __proto_h__
#define __proto_h__

/**
 * 交互协议相关定义
*/

#include "common.h"


////////////////////////////////////////////////////////////////////////////////////////////////
// WIFI状态

enum {
    WIFI_NULL = 0,
    WIFI_1,
    WIFI_2,
    WIFI_3,
    WIFI_4,
    WIFI_ERROR,
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 数据发送的结构

enum {
    S_BUF_HEAD = 0,      // 帧头
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 数据接收的结构

enum {
    R_BUF_HEAD1 = 0,      // 帧头
    R_BUF_HEAD2,          // 帧头
    R_BUF_COMM,           // 命令类型
    R_BUF_LEN,            // 数据长度
    R_BUF_NO,             // 序号
    R_BUF_DATA,           // 数据
};

enum {
    R_COMM_CTRL = 0x01,          // 控制命令
    R_COMM_OTA,                  // 升级命令
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 工作状态

enum {
    STATUS_POWEROFF,       // 关机
    STATUS_WORKING,        // 运行
    STATUS_STOP,           // 暂停
    STATUS_STANDBY,        // 待机
    STATUS_RESERVE,        // 预约
    STATUS_PREHEAT,        // 预热
    STATUS_PREHEAT_STOP,   // 预热暂停
    STATUS_PREHEAT_FINAL,  // 预热完成
};

#define FANGHUO_VALUE_MAX 80
#define FANGHUO_VALUE_MIN 60

enum {
    ERROR_TONGXUN           = 0b0000000000000001,  // [E01] 通讯故障
    ERROR_DIANJI            = 0b0000000000000010,  // [E02] 直流电机故障
    ERROR_LFANGHUO          = 0b0000000000000100,  // [E03] 左防火故障
    ERROR_LFANGHUO_MAX      = 0b0000000000001000,  // [E04] 左防火超温 (全终止)
    ERROR_RFANGHUO          = 0b0000000000010000,  // [E05] 右防火故障
    ERROR_RFANGHUO_MAX      = 0b0000000000100000,  // [E06] 右防火超温 (全终止)
    ERROR_ZHENGFAPAN        = 0b0000000001000000,  // [E07] 蒸发盘探头故障
    ERROR_ZHENGFAPAN_MAX    = 0b0000000010000000,  // [E08] 蒸发盘探头温度高于 230℃
    ERROR_ZHENGFAPAN_MIN    = 0b0000000100000000,  // [E09] 蒸发盘启动 5min 内持续低于 35℃ (保温,发酵功能屏蔽报警)
    ERROR_QIANGTI           = 0b0000001000000000,  // [E10] 腔体温度探头故障
    ERROR_QIANGTI_MAX       = 0b0000010000000000,  // [E11] 腔体温度探头温度高于 260℃
    ERROR_QIANGTI_MIN       = 0b0000100000000000,  // [E12] 腔体启动 5min 内持续低于 35℃ (保温,发酵、除垢功能屏蔽报警)

    ERROR_CHUGOU            = 0b0001000000000000,  // [A01] 除垢报警
    ERROR_QUESHUI           = 0b0010000000000000,  // [A02] 缺水报警
    ERROR_YANJI             = 0b0100000000000000,  // [A05] 烟机清洗
};

enum{
    ERROR_TYPE_DIANJI       = 0b0000000000000010,   // 风机类别故障
    ERROR_TYPE_LUZAO        = 0b0000000000111100,   // 炉灶类别故障
    ERROR_TYPE_ZHENGKAO     = 0b0010111111000000,   // 蒸烤箱类别故障

    ERROR_TYPE_ALL_DOWN     = 0b0000000000101001,   // 需要全终止
    ERROR_TYPE_APP          = 0b0000111111111111,   // 需要上报给APP的
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
    TYCOMM_WIFITEST = 0x0e,      // <-> WiFi 测试
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
    TYDPID_POWER = 1,          // 开关 <bool>
    TYDPID_FUN_SPEED = 2,                  // 风速 <enum>
    TYDPID_LOCK = 3,           // 锁 <bool>
    TYDPID_LAMP = 4,                       // 烟机照明 <bool>
    TYDPID_START = 19,             // 蒸烤 启动/暂停 <bool>
    TYDPID_END_DATE = 23,          // 预约烹饪结束时间 <value>
    TYDPID_ALL_TIME = 24,          // 烹饪时间 <value>
    TYDPID_OVER_TIME = 25,         // 剩余时间 <value>
    TYDPID_SET_TEM = 26,           // 烹饪温度 <value>
    TYDPID_NOW_TEM = 27,           // 当前温度 <value>
    TYDPID_FAULT = 31,             // 故障告警 <fault>
    TYDPID_MODE = 101,             // 蒸烤 模式选择 <enum>
    TYDPID_LAMP2 = 102,            // 蒸烤炉灯 <bool>
    TYDPID_HOB_STATUS = 103,           // 灶具状态 <raw>
    TYDPID_STATUS = 104,           // 烹饪状态 <enum>
    TYDPID_LEFT_TIMER = 105,           // 左炉头定时 <value>
    TYDPID_STOP = 106,             // 蒸烤箱停止 <value>
    TYDPID_RIGHT_TIMER = 107,          // 右炉头定时 <value>
    TYDPID_CLEAN = 108,                    // 烟机清洁启停 <bool>
    TYDPID_WARM = 109,                     // 烟机保温启停(机顶保温) <bool>
    TYDPID_RECIPE = 110,           // 菜谱模式 <value>
    // TYDPID_DOWN_TEM = 111,         // 下温度 <value>
    // TYDPID_UP_TEM = 112,           // 上温度 <value>
    TYDPID_TIMER_STATUS = 113,           // 灶具定时状态 <enum>
    TYDPID_SMOKE_TIMER = 114,              // 烟机延时显示 <value>
    TYDPID_DIY = 115,              // DIY菜谱 <raw>
    TYDPID_WARM_TIME = 116,                // 烟机保温时长(机顶保温) <value>
    TYDPID_HAND = 117,         // 手势开关 <bool>
    TYDPID_AIR_TIMER = 118,                // 定时新风 <raw>
    TYDPID_CLEAN_TIP = 119,                // 烟机清洁提示 <enum>
    TYDPID_BOOT_MODE = 120,                // 烟机联动设置 <enum>
    TYDPID_DOOR = 121,             // 门状态 <bool>
    // TYDPID_WATER = 122,            // 缺水提醒 <bool>
    TYDPID_SMOKE_SET = 123,                // 烟机延时设置 <enum>
    TYDPID_SMOKE_RUN = 124,                // [无用]烟机延时运行状态 <enum>
    TYDPID_FUN_AUTO = 125,                 // 自动巡航 <bool>
    TYDPID_ALART = 126,            // 特殊提醒
};


// 蒸烤箱状态枚举
enum {
    TYSTATUS_ZK_RESERVATION = 0,    // 预约中
    TYSTATUS_ZK_COOKING,            // 烹饪中
    TYSTATUS_ZK_CANCEL,             // 烹饪取消
    TYSTATUS_ZK_WAIT,               // 烹饪等待
    TYSTATUS_ZK_DONE,               // 烹饪完成
    TYSTATUS_ZK_PAUSE,              // 烹饪暂停
    TYSTATUS_ZK_PREHEATING,         // 预热中
    TYSTATUS_ZK_PREHEAT_WARMING,    // 预热完成保温中
};

// 灶具状态枚举
enum {
    TYSTATUS_ZJ_WAIT = 0,           // 灶具均未开火；
    TYSTATUS_ZJ_COOKING,            // 任一灶具烹饪中；
    TYSTATUS_ZJ_TIMING_END,         // 定时结束（若不需要此枚举，可忽略）
};


#endif
