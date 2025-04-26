/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-05-31 16:58:14
 * @FilePath: /cy_frame/src/viewlibs/baseItemAnimator.h
 * @Description: 基础的ItemAnimator,实现一些基础的动画效果
 * @BugList: 当前为空操作
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */

#ifndef _BASE_ITEM_ANIAMTOR_
#define _BASE_ITEM_ANIAMTOR_

#include <widgetEx/recyclerview/recyclerview.h>

class BaseItemAnimator : public RecyclerView::ItemAnimator {
public:
    bool animateDisappearance(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo* postLayoutInfo)override;
    bool animateAppearance(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo* preLayoutInfo, ItemHolderInfo& postLayoutInfo)override;
    bool animatePersistence(RecyclerView::ViewHolder& viewHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo& postLayoutInfo)override;
    bool animateChange(RecyclerView::ViewHolder& oldHolder, RecyclerView::ViewHolder& newHolder, ItemHolderInfo& preLayoutInfo, ItemHolderInfo& postLayoutInfo)override;
    void runPendingAnimations()override;
    void endAnimation(RecyclerView::ViewHolder& item)override;
    void endAnimations()override;
    bool isRunning()override;
};

#endif // _BASE_ITEM_ANIAMTOR_