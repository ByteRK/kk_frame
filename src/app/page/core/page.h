/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:37
 * @LastEditTime: 2026-01-20 15:31:00
 * @FilePath: /kk_frame/src/app/page/core/page.h
 * @Description: 页面基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PAGE_H__
#define __PAGE_H__

#include "base.h"

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
    void setBackBtn(int id);         // 设置返回按钮
};

/// @brief 页面构建器
class PageCreator {
protected:
    typedef std::function<PageBase* ()> CallBack;
    static  std::map<int8_t, CallBack>  sPage;
public:
    static void      add(int8_t page_id, CallBack sink) {
        sPage.insert(std::make_pair(page_id, sink));
    }
    static PageBase* get(int8_t page_id) {
        auto it = sPage.find(page_id);
        if (it == sPage.end()) return nullptr;
        return it->second();
    }
};

// 页面注册模板
template<typename T>
class PageRegister {
public:
    PageRegister(int8_t page) {
        PageCreator::add(page, []() {return new T();});
    }
};

// 页面注册宏，用于简化注册，直接于.cc文件中定义
#define PAGE_REGISTER(ID,CLASS) static PageRegister<CLASS> __P_R_##CLASS(ID);

#endif // !__PAGE_H__