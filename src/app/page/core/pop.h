/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:49
 * @LastEditTime: 2026-06-09 00:52:59
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
    ViewGroup*     mPopRootView;     // 弹窗根节点

private:
    bool           mIsGauss;         // 是否有模糊背景
    int            mGaussRadius;     // 模糊背景圆角
    int            mGaussColor;      // 模糊背景颜色
    bool           mPageDisplay;     // 底层页面显示
    Runnable       mPageDspRunner;   // 底层页面显示回调

public:
    PopBase(std::string resource);   // 构造函数
    virtual ~PopBase();              // 析构函数

    View* getRootView() override;    // 获取根节点

    void callAttach() override;      // 挂载
    void callDetach() override;      // 卸载

protected:
    void close();                    // 关闭弹窗

    void setMargin(int start, int top, int end, int bottom);   // 设置外边距
    void setPadding(int start, int top, int end, int bottom);  // 设置内边距
    void setGauss(int radius = 10, int color = 0x99000000);    // 设置模糊背景
    void setColor(int color = 0x99000000);                     // 设置背景色
    void setPageDisplay(bool show);                            // 设置底层页面显示

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