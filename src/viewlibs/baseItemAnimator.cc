/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:54:18
 * @FilePath: /hana_frame/src/viewlibs/baseItemAnimator.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

 
#include "baseItemAnimator.h"
#include <cdlog.h>

/// @brief 实现项消失时的动画效果
/// @param viewHolder 
/// @param preLayoutInfo 
/// @param postLayoutInfo 
/// @return 
bool BaseItemAnimator::animateDisappearance(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo* postLayoutInfo) {
    LOGV("animateDisappearance  实现项消失时的动画效果");
    return false;
}

/// @brief 实现项出现时的动画效果
/// @param viewHolder 
/// @param preLayoutInfo 
/// @param postLayoutInfo 
/// @return 
bool BaseItemAnimator::animateAppearance(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo* preLayoutInfo, ItemHolderInfo& postLayoutInfo) {
    LOGV("animateAppearance  实现项出现时的动画效果");
    return false;
}

/// @brief 实现项保持不变时的动画效果
/// @param viewHolder 
/// @param preLayoutInfo 
/// @param postLayoutInfo 
/// @return 
bool BaseItemAnimator::animatePersistence(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo& postLayoutInfo) {
    LOGV("In BaseItemAnimator::animatePersistence ---------");
    return false;
}

/// @brief 实现项更改时的动画效果
/// @param oldHolder 
/// @param newHolder 
/// @param preLayoutInfo 
/// @param postLayoutInfo 
/// @return 
bool BaseItemAnimator::animateChange(RecyclerView::ViewHolder& oldHolder, RecyclerView::ViewHolder& newHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo& postLayoutInfo) {
    LOGV("animateChange  实现项更改时的动画效果");
    return false;
}

/// @brief 运行挂起的动画
void BaseItemAnimator::runPendingAnimations() {
    LOGV("runPendingAnimations  运行挂起的动画");
}

/// @brief 结束指定项的动画
/// @param item 
void BaseItemAnimator::endAnimation(RecyclerView::ViewHolder& item) {
    LOGV("endAnimation  结束指定项的动画");
}

/// @brief 结束所有动画
void BaseItemAnimator::endAnimations() {
    LOGV("endAnimations  结束所有动画");
}

/// @brief 返回动画是否正在运行
/// @return 
bool BaseItemAnimator::isRunning() {
    LOGV("isRunning  返回动画是否正在运行");
    return false;
}