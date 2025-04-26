/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-05-31 16:58:41
 * @FilePath: /cy_frame/src/viewlibs/baseItemAnimator.cc
 * @Description: 基础的ItemAnimator,实现一些基础的动画效果
 * @BugList: 当前为空操作
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
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