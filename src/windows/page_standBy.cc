/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2024-12-13 18:43:00
 * @FilePath: /kk_frame/src/windows/page_standBy.cc
 * @Description: 待机页面
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#include "page_standBy.h"
#include "manage.h"

#include "btn_mgr.h"
#include "global_data.h"
#include "tuya_mgr.h"

static std::map<std::string, std::string> gWaterCodeImgList = {
    {"101","@mipmap/weather/dayu"},       // 大雨
    {"102","@mipmap/weather/lei"},        // 雷暴
    {"103","@mipmap/weather/yangsha"},    // 沙尘暴
    {"104","@mipmap/weather/xiaoxue"},    // 小雪
    {"105","@mipmap/weather/zhongxue"},   // 雪
    {"106","@mipmap/weather/wu"},         // 冻雾
    {"107","@mipmap/weather/dayu"},       // 暴雨
    {"108","@mipmap/weather/zhongyu"},    // 局部阵雨
    {"109","@mipmap/weather/yangsha"},    // 浮尘
    {"110","@mipmap/weather/lei"},        // 雷电
    {"111","@mipmap/weather/xiaoyu"},     // 小阵雨
    {"112","@mipmap/weather/zhongyu"},    // 雨
    {"113","@mipmap/weather/yujiaxue"},   // 雨夹雪
    {"114","@mipmap/weather/fengbao"},    // 尘卷风
    {"115","@mipmap/weather/xiaoxue"},    // 冰粒
    {"116","@mipmap/weather/yangsha"},    // 强沙尘暴
    {"117","@mipmap/weather/yangsha"},    // 扬沙
    {"118","@mipmap/weather/zhongyu"},    // 小到中雨
    {"119","@mipmap/weather/qing"},       // 大部晴朗
    {"120","@mipmap/weather/qing"},       // 晴
    {"121","@mipmap/weather/wu"},         // 雾
    {"122","@mipmap/weather/zhenyu"},     // 阵雨
    {"123","@mipmap/weather/zhenyu"},     // 强阵雨
    {"124","@mipmap/weather/daxue"},      // 大雪
    {"125","@mipmap/weather/dayu"},       // 特大暴雨
    {"126","@mipmap/weather/baoxue"},     // 暴雪
    {"127","@mipmap/weather/daxue"},      // 冰雹
    {"128","@mipmap/weather/zhongxue"},   // 小到中雪
    {"129","@mipmap/weather/shaoyun"},    // 少云
    {"130","@mipmap/weather/xiaoxue"},    // 小阵雪
    {"131","@mipmap/weather/zhongxue"},   // 中雪
    {"132","@mipmap/weather/ying"},        // 阴
    {"133","@mipmap/weather/xiaoxue"},    // 冰针
    {"134","@mipmap/weather/dayu"},       // 大暴雨
    // {"135","@mipmap/weather/wind"},
    {"136","@mipmap/weather/zhenyu"},     // 雷阵雨伴有冰雹
    {"137","@mipmap/weather/yujiaxue"},   // 冻雨
    {"138","@mipmap/weather/zhongxue"},   // 阵雪
    {"139","@mipmap/weather/xiaoyu"},     //小雨
    {"140","@mipmap/weather/mai"},        // 霾
    {"141","@mipmap/weather/zhongyu"},    // 中雨
    {"142","@mipmap/weather/duoyun"},     // 多云
    {"143","@mipmap/weather/leizhenyu"},  // 雷阵雨
    {"144","@mipmap/weather/dayu"},       // 中到大雨
    {"145","@mipmap/weather/dayu"},       // 大到暴雨
    {"146","@mipmap/weather/qing"},       // 晴朗
};

static const std::string WEEK_LIST[7] = { "星期日","星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };


StandByPage::StandByPage() :PageBase("@layout/page_standby") {
    initUI();
    mLastTick = SystemClock::uptimeMillis();
}

StandByPage::~StandByPage() {
}

void StandByPage::onReload() {
    LOGI("StandByPage::reload");
    mLastClick = SystemClock::uptimeMillis();
    mLastTick = 0;
    onTick();
}

void StandByPage::onCheckLight(uint8_t* left, uint8_t* right) {
    memset(left, BTN_HIGHT, 7);
    memset(right, BTN_HIGHT, 8);
}

void StandByPage::onTick() {
}

uint8_t StandByPage::getType() const {
    return PAGE_STANDBY;
}

void StandByPage::checkLight(uint8_t* left, uint8_t* right) {
    memset(left, BTN_HIGHT, 7);
    memset(right, BTN_HIGHT, 8);
}

void StandByPage::getView() {
}

void StandByPage::setAnim() {
}

void StandByPage::setView() {
}

void StandByPage::loadData() {
}

/// @brief 按键监听
/// @param keyCode 
/// @param status 
/// @return 是否响铃
bool StandByPage::onKey(uint16_t keyCode, uint8_t status) {
    return false;
}