/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:54:00
 * @FilePath: /hana_frame/src/viewlibs/baseItemAnimator.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
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