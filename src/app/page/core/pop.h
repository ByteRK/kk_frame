/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:49
 * @LastEditTime: 2026-07-03 14:37:27
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
private:
    constexpr static uint64_t BG_COLOR = 0x99000000;   // 默认背景颜色

protected:
    ViewGroup*     mPopRootView{ nullptr };     // 弹窗根节点

private:
    bool           mIsGauss{ false };           // 是否有模糊背景
    int            mGaussRadius{ 10 };          // 模糊背景圆角
    uint64_t       mGaussColor{ BG_COLOR };     // 模糊背景颜色
    bool           mPageDisplay{ false };       // 底层页面显示
    Runnable       mPageDspRunner;              // 底层页面显示回调

public:
    PopBase(std::string resource);   // 构造函数
    virtual ~PopBase();              // 析构函数

    View* getRootView() override;    // 获取根节点

    void callAttach() override;      // 挂载
    void callDetach() override;      // 卸载

protected:
    void close();                    // 关闭弹窗
    void back();                     // 返回上一层弹窗

    void setMargin(int start, int top, int end, int bottom);      // 设置外边距
    void setPadding(int start, int top, int end, int bottom);     // 设置内边距
    void setGauss(int radius = 10, uint64_t color = BG_COLOR);    // 设置模糊背景
    void setColor(uint64_t color = BG_COLOR);                     // 设置背景色
    void setPageDisplay(bool show);                               // 设置底层页面显示

private:
    void applyGauss();               // 应用模糊背景
};

// 构建器标签
struct PopCreatorTag { };
// 弹窗构建器 
typedef Creator<PopBase, PopCreatorTag> PopCreator;
// 弹窗注册宏，用于简化注册，直接于.cc文件中定义
#define POP_REGISTER(ID, CLASS) static Register<PopCreator, CLASS> __POP_REG_##CLASS(ID)

#endif