/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:49
 * @LastEditTime: 2026-01-20 16:20:26
 * @FilePath: /kk_frame/src/app/page/core/pop.h
 * @Description: 弹窗基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __POP_H__
#define __POP_H__

#include "base.h"

/// @brief 弹窗基类
class PopBase :public PBase {
protected:
    ViewGroup* mPopRootView;         // 弹窗根节点
    
public:
    PopBase(std::string resource);   // 构造函数
    virtual ~PopBase();              // 析构函数

    View* getRootView() override;    // 获取根节点

protected:
    void onAttach() override;        // 挂载
    void onDetach() override;        // 卸载

    void setMargin(int start, int top, int end, int bottom);   // 设置边距
    void setGlass(int radius = 10, uint color = 0x99000000);   // 设置模糊背景
    void setColor(int color = 0x99000000);                     // 设置背景色
};

/// @brief 弹窗构建器
class PopCreator {
protected:
    typedef std::function<PopBase* ()> CallBack;
    static  std::map<int8_t, CallBack>  sPop;
public:
    static void     add(int8_t pop_id, CallBack sink) {
        sPop.insert(std::make_pair(pop_id, sink));
    }
    static PopBase* get(int8_t pop_id) {
        auto it = sPop.find(pop_id);
        if (it == sPop.end()) return nullptr;
        return it->second();
    }
};

// 弹窗注册模板
template<typename T>
class PopRegister {
public:
    PopRegister(int8_t pop) {
        PopCreator::add(pop, []() {return new T();});
    }
};

// 弹窗注册宏，用于简化注册，直接于.cc文件中定义
#define POP_REGISTER(ID,CLASS) static PopRegister<CLASS> __P_R_##CLASS(ID);

#endif