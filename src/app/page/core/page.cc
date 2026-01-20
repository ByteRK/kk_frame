/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:43
 * @LastEditTime: 2026-01-20 16:26:32
 * @FilePath: /kk_frame/src/app/page/core/page.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page.h"
#include "wind_mgr.h"
#include <widget/imageview.h>

// 静态变量定义
std::map<int8_t, PageCreator::CallBack> PageCreator::sPage;

/// @brief 构造
/// @param resource 资源路径
PageBase::PageBase(std::string resource) :PBase(resource) {
}

/// @brief 析构
PageBase::~PageBase() {
}

/// @brief 是否允许自动回收
/// @return 
bool PageBase::canAutoRecycle() const {
    return false;
}

/// @brief 初始化UI
void PageBase::initUI() {
    mInitUIFinish = false;
    getView();
    setAnim();
    setView();
    mInitUIFinish = true;
}

/// @brief 设置返回按钮
/// @param id 返回按钮id
void PageBase::setBackBtn(int id) {
    View* v = get<View>(id);
    if (v) {
        v->setOnClickListener([](View&) {g_windMgr->goToPageBack();});
        ImageView* iv = dynamic_cast<ImageView*>(v);
        if (iv && iv->getDrawable())
            iv->getDrawable()->setFilterBitmap(true);
    }
}