/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2024-12-13 18:59:01
 * @FilePath: /kk_frame/src/windows/base.h
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _BASE_H_
#define _BASE_H_

#include <string.h>
#include <view/view.h>
#include "json_func.h"

#define __get(I)         findViewById(I)
#define __getv(T, I)     (dynamic_cast<T *>(findViewById(I)))
#define __getg(G, I)     (G)->findViewById(I)
#define __getgv(G, T, I) (dynamic_cast<T *>((G)->findViewById(I)))

#define __click(I, L)     __get(I)->setOnClickListener(L)
#define __clickv(V, L)    (V)->setOnClickListener(L)
#define __clickg(G ,I, L) __getg(G, I)->setOnClickListener((L))

#define __able(I)        __get(I)->setEnabled(true)
#define __unable(I)      __get(I)->setEnabled(false)
#define __ablev(V)       (V)->setEnabled(true)
#define __unablev(V)     (V)->setEnabled(false)
#define __ableg(G, I)    __getg(G,I)->setEnabled(true)
#define __unableg(G, I)  __getg(G,I)->setEnabled(false)

#define __visible(I,S)    __get(I)->setVisibility(S)
#define __visiblev(V,S)   (V)->setVisibility(S)
#define __visibleg(G,I,S) __getg(G,I)->setVisibility(S)

#define __delete(v)      if(v)delete v;

// 语言定义
enum {
    LANG_ZH_CN,
    LANG_ZH_TC,
    LANG_EN_US
};

// 页面定义
enum {
    PAGE_NULL,       // 空状态
    PAGE_STANDBY,    // 待机
    PAGE_OTA,        // OTA
};

// 弹窗定义
enum {
    POP_NULL,        // 空状态
    POP_LOCK,        // 童锁
    POP_TIP,         // 提示
};

/*
 *************************************** 基类 ***************************************
 */

 /// @brief 基类
class PBase {
protected:
    Looper*         mLooper = nullptr;     // 循环
    Context*        mContext = nullptr;    // 上下文
    LayoutInflater* mInflater = nullptr;   // 布局加载器
    uint8_t         mLang = LANG_ZH_CN;    // 语言

    ViewGroup*      mRootView = nullptr;   // 根节点
    uint64_t        mLastTick = 0;         // 上次Tick时间
    uint64_t        mLastClick = 0;        // 上次按键点击时间
public:
    PBase();
    virtual ~PBase();

    uint8_t getLang() const;
    View* getRootView();
    virtual uint8_t getType() const = 0;

    void callTick();
    void callAttach();
    void callDetach();
    void callMcu(uint8_t* data, uint8_t len);
    bool callKey(uint16_t keyCode, uint8_t evt);
    void callLangChange(uint8_t lang);
    void callCheckLight(uint8_t* left, uint8_t* right);
protected:
    virtual void initUI() = 0;
    View* findViewById(int id);

    virtual void onTick();
    virtual void onReload();
    virtual void onDetach();
    virtual void onMcu(uint8_t* data, uint8_t len);
    virtual bool onKey(uint16_t keyCode, uint8_t evt);
    virtual void onLangChange();
    virtual void onCheckLight(uint8_t* left, uint8_t* right);

    void setLangText(TextView* v, const Json::Value& value);
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
    bool mInitUIFinish = false;    // UI是否初始化完成
public:
    PageBase(std::string resource);
    virtual ~PageBase();
protected:
    void initUI() override;
    virtual void getView() = 0;
    virtual void setAnim() = 0;
    virtual void setView() = 0;
    virtual void loadData() = 0;
private:
};

#endif
