/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-26 14:24:20
 * @FilePath: /hana_frame/src/data/struct.h
 * @Description:
 * Copyright (c) 2025 by hanakami, All Rights Reserved.
 */

#ifndef _STRUCT_H_
#define _STRUCT_H_

#include "common.h"

 /// @brief 主功能大类
 enum {
    MENU_NULL,              // 空

    MENU_STEAM,             // 专业蒸制
    MENU_BAKED,             // 专业烘烤
    MENU_ROAST,             // 空气炸
    MENU_COOKBOOK,          // 智能菜谱
    MENU_ADDFUNS,           // 辅助功能

    MENU_MEN,               // 焖制
};

/// @brief 主功能ID
enum {
    MODE_NULL,                   // 空

    MODE_BAKED_UP_DOWN = 1,      // 上下烤
    MODE_BAKED_UP = 2,           // 上烤 


    MODE_EMUN_DIY = 254,         // DIY模式

    MODE_TEST = 0xFF,            // 测试
};
/// @brief 菜谱类别
enum {
    COOKBOOL_LASET = 0,     // 最近
    COOKBOOK_MEAT = 1,      // 肉类
    COOKBOOK_EGG,           // 蛋类
    COOKBOOK_FISH,          // 鱼类海鲜
    COOKBOOK_RICE,          // 米面
    COOKBOOK_VEGETABLE,     // 蔬菜杂粮
    COOKBOOK_CHINESE,       // 中式特色
    COOKBOOK_WESTERN,       // 西点

    COOKBOOK_CLOUD,         // 云端菜谱
};



enum class MicroLevel : uint8_t { Null = 0, Min = 1, Max = 10 };
enum class SteamLevel : uint8_t { Null = 0, Min = 1, Max = 5 };

/// @brief 步骤结构体
typedef struct ModeItem {
    uint8_t     mode = 0;        // 模式
    uint32_t    tem = 0;         // 温度
    uint32_t    time = 0;        // 时间
    MicroLevel     micro = MicroLevel::Null;       // 微波档位
    SteamLevel     steam = SteamLevel::Null;       // 蒸汽档位
    bool        hurrRoast;   // 飓风烤
    bool        autoNext;    // 是否自动进入下一步骤
    std::string toNextText;  // 进入下一步的文案（非自动进入则不显示）

    ModeItem(uint8_t _mode, uint32_t _tem, uint32_t _time) 
        : ModeItem(_mode, _tem, _time, MicroLevel::Null, SteamLevel::Null) {}

    ModeItem(uint8_t _mode, uint32_t _tem, uint32_t _time, 
             MicroLevel _micro, SteamLevel _steam, bool _hurrRoast = false,
             bool _autoNext = true, std::string _toNextText = "")
        : mode(_mode), tem(_tem), time(_time), micro(_micro), steam(_steam),
          hurrRoast(_hurrRoast), autoNext(_autoNext), toNextText(std::move(_toNextText)) {}
}ModeItem;


/// @brief 主功能模式数据结构体
typedef struct Mode_Struct {
    /// 公用数据
    uint8_t     parent;         // 父类别
    uint8_t     id;             // id
    std::string name;           // 名称
    std::string endTip;         // 结束提示
    std::string imageKey;       // 图片资源关键字
    std::string disc;           // 描述

    /// 常规模式 特定数据
    uint32_t    minTem;         // 最小温度（如为上下温，则高16位为上管温度，低16位为下管温度）
    uint32_t    maxTem;         // 最大温度（如为上下温，则高16位为上管温度，低16位为下管温度）
    uint32_t    minTime;        // 最小时间
    uint32_t    maxTime;        // 最大时间
    uint8_t     minMicro;       // 微波最小档位
    uint8_t     maxMicro;       // 微波最大档位
    uint8_t     minSteam;       // 蒸汽最小档位
    uint8_t     maxSteam;       // 蒸汽最大档位
    bool        canPreheat;     // 是否支持预热

    /// 菜谱 特定数据
    uint8_t       cookbookParent;// 菜谱父类别
    std::string   seasoning;    // 调料
    std::string   tools;        // 工具
    std::string   stepsText;    // 步骤描述
    std::string   introduction; // 菜品介绍
    /// 运行特定数据
    bool        preheat;        // 预热
    std::vector<ModeItem> modes;    // 步骤列表

    /// 空 构造函数
    Mode_Struct() { };

    /// 常规模式 构造函数
    Mode_Struct(uint8_t _parent, uint8_t _id, std::string _name, std::string _imageKey,
        uint32_t _minTem, uint32_t _maxTem, uint32_t _minTime, uint32_t _maxTime,
        uint8_t _minMicro, uint8_t _maxMicro, uint8_t _minSteam, uint8_t _maxSteam,
        bool _canPreheat, bool _preheat, bool _canHurrRoast,
        std::vector<ModeItem> _modes) :
        parent(_parent), id(_id), name(_name), imageKey(_imageKey),
        minTem(_minTem), maxTem(_maxTem), minTime(_minTime), maxTime(_maxTime),
        minMicro(_minMicro), maxMicro(_maxMicro), minSteam(_minSteam), maxSteam(_maxSteam),
        canPreheat(_canPreheat), preheat(_preheat) {

        for (auto modeStep : _modes) {
            modeStep.mode = _id;
        }
        modes = _modes;

        endTip = "高温防烫，请缓慢打开门体！\n取出食物后，请及时清理内腔残渣和底部积水，并将内腔烘干处理，做好清洁保养！";
    };

    /// 本地菜谱 构造函数
    Mode_Struct(uint8_t _parent, uint8_t _id, std::string _name,
        uint8_t _cookbookParent, bool _preheat, std::vector<ModeItem> _modes,
        std::string _seasoning, std::string _tools, std::string _stepsText,
        std::string _introduction, std::string _qrcode, std::string _video) :
        parent(_parent), id(_id), name(_name), cookbookParent(_cookbookParent), preheat(_preheat),
        seasoning(_seasoning), tools(_tools), stepsText(_stepsText), introduction(_introduction) {

        endTip = "高温防烫，请缓慢打开门体！\n取出食物后，请及时清理内腔残渣和底部积水，并将内腔烘干处理，做好清洁保养！";
    };

    /// @brief 获取总工作时间
    /// @return 分钟
    uint32_t getWorkTime()const {
        uint32_t time = 0;
        if (modes.size() <= 0) std::runtime_error("ModeStruct::isUpDownTem() modes.size() <= 0 !!!!!");
        for (auto& item : modes)time += item.time;
        return time;
    };

    /// @brief 获取工作时间
    /// @return xx:xx
    std::string stringTime() {
        uint32_t time = getWorkTime();
        return std::to_string(time / 60) + ":" + std::to_string(time % 60);
    };

    /// @brief 是否有步骤
    bool isTrue()const {
        return modes.size() > 0;
    }

    /// @brief 获取背景图路径
    /// @param with_param 是否带参数圆环
    /// @return 
    std::string getBg(bool with_param = false)const {
        if (with_param)return "@mipmap/bg/bg2_" + imageKey;
        return "@mipmap/bg/bg_" + imageKey;
    };

    /// @brief 获取图标路径
    /// @param center true: 中心图标 false: 侧边图标
    /// @return 
    std::string getImg(bool center = false)const {
        if (center)return "@mipmap/img/img_" + imageKey;
        return "@mipmap/img/img2_" + imageKey;
    };

    /// @brief 是否上下温 (常规模式专用)
    /// @return 
    bool isUpDownTem()const {
        if (modes.size() <= 0) std::runtime_error("ModeStruct::isUpDownTem() modes.size() <= 0 !!!!!");
        return (modes[0].tem >> 16) != 0;
    }

    /// @brief 重置模式信息 仅DIY生效 (预留，项目专用。用作清除涂鸦的缓存)
    void reset() {
        // 暂未对项目进行专用调整
        id = 0;
        name = "DIY模式";
        imageKey = "";
        preheat = false;
        seasoning = "";
        tools = "";
        stepsText = "";
        modes.clear();
    }
}ModeStruct;
#endif