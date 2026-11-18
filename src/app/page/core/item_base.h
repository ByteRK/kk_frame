/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-18 16:10:00
 * @LastEditTime: 2026-11-18 16:04:35
 * @FilePath: /kk_frame/src/app/page/core/item_base.h
 * @Description: 容器子项基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __ITEM_BASE_H__
#define __ITEM_BASE_H__

#include <string>

#include "app_common.h"

#include <view/view.h>
#include <view/viewgroup.h>

/// @brief 容器子项基类
/// @note 挂载/摘除由切换器通过 callShow/callHide 统一驱动，子类只实现 onShow/onHide
class ItemBase {
protected:
    ViewGroup* mContainer{ nullptr };   // 挂载容器
    View*      mRootView{ nullptr };    // 根视图

public:
    ItemBase() = default;
    explicit ItemBase(const std::string& layout);
    virtual  ~ItemBase();

public:
    void callShow();
    void callHide();
    void setContainer(ViewGroup* container);

protected:
    virtual void onShow() { };
    virtual void onHide() { };

    View* inflateItem(const std::string& layout, bool attachToContainer = false);

private:
    void attachView();
    void detachView();
};

#endif // !__ITEM_BASE_H__
