/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:26
 * @LastEditTime: 2024-08-23 13:59:38
 * @FilePath: /kk_frame/src/windows/base.h
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _BASE_H_
#define _BASE_H_

#include <cdlog.h>

#include "proto.h"
#include "common.h"
#include "R.h"

#include <widget/relativelayout.h>
#include "rvNumberPicker.h"

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

 // 页面定义
enum {
    PAGE_NULL,       // 空状态
    PAGE_STANDBY,    // 待机
};

// 弹窗定义
enum {
    POP_NULL,        // 空状态
    POP_LOCK,        // 童锁
    POP_TIP,         // 提示
};

/// @brief 页面基类
class PopBase {
public:
    ViewGroup* mPopView = nullptr;
protected:
    Context* mContext;
    LayoutInflater* mInflater;
public:
    PopBase(Context* context)
        : mContext(context), mInflater(nullptr) {
    };
    PopBase(Context* context, std::string resource)
        : mContext(context), mInflater(LayoutInflater::from(context)) {
        mPopView = (ViewGroup*)mInflater->inflate(resource, nullptr);
    };
    virtual ~PopBase() {
        if (mPopView)delete mPopView;
    }
    virtual void close() = 0;
    virtual int8_t getType() = 0;
    virtual bool checkLight(uint8_t* left, uint8_t* right) = 0;
    virtual void onTick() = 0;
    virtual bool onKey(uint16_t keyCode, uint8_t status) = 0;
};

/// @brief 页面基类
class PageBase :public RelativeLayout {
public:

protected:
    Looper*         mloop;
    Context*        mContext;
    LayoutInflater* mInflater;

    ViewGroup*      mRootView;        // 根布局
    bool            mInitUIFinish;    // UI是否初始化完成

    uint64_t        mLastTick = 0;    // 上次刷新时间
    uint64_t        mLastClick = 0;   // 上次点击时间
private:
    Runnable        mGogoHomeRunner;
    Runnable        mGoToBackRunner;
public:
    PageBase(std::string resource);
    virtual ~PageBase();

    virtual void reload();
    virtual void onTick();
    virtual int8_t getPageType() = 0;
    virtual void checkLight(uint8_t* left, uint8_t* right) = 0;

    bool baseOnKey(uint16_t keyCode, uint8_t status);
    virtual bool onKey(uint16_t keyCode, uint8_t status);
    
protected:
    void initUI();

    virtual void getView() = 0;
    virtual void setAnim() = 0;
    virtual void setView() = 0;
    virtual void loadData() = 0;

    View* getWifiView();

    void goToBack(uint32_t delay = 0);
    void goToHome(uint32_t delay = 0);

    void hideAll();
public:
    PopBase* getPop();
    bool showPop(int8_t type);
    void removePop();
    bool showBlack(bool upload = true);
    void removeBlack();
    void showPopText(std::string text, int8_t level, bool animate = true, bool lock = false);
    void removePopText();
private:
};

#endif
