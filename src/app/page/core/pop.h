/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:49
 * @LastEditTime: 2026-01-04 14:38:45
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
    ViewGroup* mPopRootView;         // 弹窗根节点
public:
    PopBase(std::string resource);   // 构造函数
    virtual ~PopBase();              // 析构函数

    View* getRootView() override;    // 获取根节点

protected:
    void setGlass(int radius = 10, uint color = 0x99000000);   // 设置模糊背景
    void setColor(int color = 0x99000000);                     // 设置背景色
};

/// @brief 弹窗注册（防止交叉include）
/// @param pop 
/// @param func 
void registerPopToMgr(int8_t pop, std::function<PopBase* ()> func);

// 弹窗注册模板
template<typename T>
class PopRegister {
public:
    PopRegister(int8_t pop) {
        registerPopToMgr(pop, []() {return new T();});
    }
};

// 弹窗注册宏，用于简化注册，直接于.cc文件中定义
#define POP_REGISTER(K,C) static PopRegister<C> pop_register_##C(K);

#endif