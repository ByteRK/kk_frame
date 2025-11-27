/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2025-11-27 11:44:52
 * @FilePath: /cy_frame/src/windows/base.h
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2024 by Cy, All Rights Reserved.
 *
**/


#ifndef _BASE_H_
#define _BASE_H_

#include <string.h>
#include <view/view.h>
#include <widget/textview.h>

#include "R.h"
#include "msg.h"
#include "json_func.h"

#define __get(I)         findViewById(I)
#define __getv(T, I)     (dynamic_cast<T *>(findViewById(I)))
#define __getg(G, I)     (G)->findViewById(I)
#define __getgv(G, T, I) (dynamic_cast<T *>((G)->findViewById(I)))

#define __click(I, L)     __get(I)->setOnClickListener(L)
#define __clickv(V, L)    (V)->setOnClickListener(L)
#define __clickg(G ,I, L) __getg(G, I)->setOnClickListener((L))

#define __visible(I,S)    __get(I)->setVisibility(S)
#define __visiblev(V,S)   (V)->setVisibility(S)
#define __visibleg(G,I,S) __getg(G,I)->setVisibility(S)

template<typename T>
inline void safeDelete(T*& p) {
    if (p) { delete p; p = nullptr; }
}

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
        "中文",
        "繁体",
        "English"
    };
    return langText[lang % LANG_MAX];
};

/*
 *************************************** 基类 ***************************************
 */

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
private:
    uint32_t        mAutoExit = 0;                             // 自动退出到待机的时间
    bool            mAutoExitWithBlack = false;                // 自动退出到待机时是否显示黑屏
public:
    PBase(std::string resource);                               // 构造函数
    virtual ~PBase();                                          // 析构函数

    uint8_t getLang() const;                                   // 获取当前页面语言
    View*   getRootView();                                     // 获取根节点
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
    View* findViewById(int id);                                // 查找View(弥补非继承自View)

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
    void setAutoBackToStandby(uint32_t time, bool withBlack = false);  // 设置自动退出到待机
    void setLangText(TextView* v, const Json::Value& value);           // 设置语言文本
};

/*
 *************************************** 弹窗 ***************************************
 */

 /// @brief 页面基类
class PopBase :public PBase {
public:
    PopBase(std::string resource);
    virtual ~PopBase();
};

/*
 *************************************** 页面 ***************************************
 */

 /// @brief 页面基类
class PageBase :public PBase {
protected:
    bool mInitUIFinish = false;     // UI是否初始化完成
public:
    PageBase(std::string resource); // 构造函数
    virtual ~PageBase();            // 析构函数

    virtual bool canAutoRecycle() const; // 是否允许自动回收
protected:
    void initUI() override;
    virtual void getView() { };      // 获取页面指针
    virtual void setAnim() { };      // 设置动画属性
    virtual void setView() { };      // 设置页面属性
};

#endif
