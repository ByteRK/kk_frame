/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-12-16 09:51:02
 * @LastEditTime: 2025-12-26 11:51:41
 * @FilePath: /kk_frame/src/utils/tuya_utils.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "tuya_utils.h"
#include <unordered_map>
#include <algorithm>

std::string TuyaUtils::getWeatherIcon(const std::string& code) {
    static std::unordered_map<std::string, std::string> sWaterCodeImgList = {
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
    
    auto weather = sWaterCodeImgList.find(code);
    return weather != sWaterCodeImgList.end() ? weather->second : "@mipmap/weather/qing";
};
