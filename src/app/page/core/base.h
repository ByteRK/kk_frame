/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2026-01-13 09:22:40
 * @FilePath: /kk_frame/src/app/page/core/base.h
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BASE_H__
#define __BASE_H__

#include "R.h"
#include "msg.h"
#include "json_utils.h"
#include "project_utils.h"
#include "app_common.h"
#include "app_version.h"

#include <view/view.h>
#include <view/viewgroup.h>
#include <widget/textview.h>

// 语言定义
enum {
    LANG_ZH_CN,      // 简体中文
    LANG_ZH_TC,      // 繁体中文
    LANG_EN_US,      // 英文
    LANG_MAX,        // 语言数量
};

// 页面定义
enum {
    PAGE_NULL,       // 空状态
    PAGE_DEMO,       // 示例页面
    PAGE_HOME,       // 主页面
    PAGE_OTA,        // OTA
};

// 弹窗定义
enum {
    POP_NULL,        // 空状态
    POP_LOCK,        // 童锁
    POP_TIP,         // 提示
};

/// @brief 语言转文本
/// @param lang 语言
/// @return 
static std::string langToText(uint8_t lang) {
    const static char* langText[] = {
        "中文", "繁体", "English"
    };
    return langText[lang % LANG_MAX];
};

/*
 *************************************** 基类 ***************************************
**/

namespace AppRid = APP_NAME::R::id;

/// @brief 基类
class PBase {
protected:
    Looper*         mLooper = nullptr;                         // 事件循环
    Context*        mContext = nullptr;                        // 上下文
    LayoutInflater* mInflater = nullptr;                       // 布局加载器
    uint8_t         mLang = LANG_ZH_CN;                        // 语言

    ViewGroup*      mRootView = nullptr;                       // 根节点
    uint64_t        mLastTick = 0;                             // 上次Tick时间
    bool            mIsAttach = false;                         // 是否已经Attach
public:
    PBase(std::string resource);                               // 构造函数
    virtual ~PBase();                                          // 析构函数

    uint8_t getLang() const;                                   // 获取当前页面语言
    virtual View* getRootView();                               // 获取根节点
    virtual int8_t getType() const = 0;                        // 获取页面类型

    void callTick();                                           // 调用定时器
    void callAttach();                                         // 通知页面挂载
    void callDetach();                                         // 通知页面剥离
    void callLoad(LoadMsgBase* loadMsg);                       // 调用重加载
    SaveMsgBase* callSaveState();                              // 保存状态
    void callRestoreState(const SaveMsgBase* saveMsg);         // 恢复状态
    void callMsg(const RunMsgBase* runMsg);                    // 运行时消息
    void callMcu(uint8_t* data, uint8_t len);                  // 接受电控数据
    bool callKey(uint16_t keyCode, uint8_t evt);               // 接受按键事件
    void callLangChange(uint8_t lang);                         // 调用语言切换
    void callCheckLight(uint8_t* left, uint8_t* right);        // 调用检查按键灯
protected:
    virtual void initUI() = 0;                                 // 初始化UI
    virtual void onTick();                                     // 定时器回调
    virtual void onAttach();                                   // 挂载页面回调
    virtual void onDetach();                                   // 剥离页面回调
    virtual void onLoad(LoadMsgBase* loadMsg);                 // 数据加载回调
    virtual SaveMsgBase* onSaveState();                        // 状态保存
    virtual void onRestoreState(const SaveMsgBase* saveMsg);   // 状态恢复
    virtual void onMsg(const RunMsgBase* runMsg);              // 运行时消息回调
    virtual void onMcu(uint8_t* data, uint8_t len);            // 电控数据回调
    virtual bool onKey(uint16_t keyCode, uint8_t evt);         // 按键事件回调
    virtual void onLangChange();                               // 语言切换通知回调
    virtual void onCheckLight(uint8_t* left, uint8_t* right);  // 检查按键灯回调
    void setLangText(TextView* v, const Json::Value& value);   // 设置语言文本
protected:
    /// @brief 获取控件指针
    /// @tparam T 
    /// @param id 
    /// @return 
    template <typename T>
    T* get(int id) {
        return dynamic_cast<T*>(mRootView->findViewById(id));
    }

    /// @brief 获取控件指针
    /// @tparam T 
    /// @param vparent 
    /// @param id 
    /// @return 
    template <typename T>
    static T* get(View* vparent, int id) {
        return dynamic_cast<T*>(vparent->findViewById(id));
    }
};

#endif // !__BASE_H__
