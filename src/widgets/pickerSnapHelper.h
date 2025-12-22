/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-05-31 16:57:27
 * @FilePath: /kk_frame/src/viewlibs/pickerSnapHelper.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __pickerSnapHelper_H__
#define __pickerSnapHelper_H__

#include <widgetEx/recyclerview/snaphelper.h>
#include <widgetEx/recyclerview/orientationhelper.h>

class PickerSnapHelper :public SnapHelper {
private:
    static constexpr float INVALID_DISTANCE = 1.f;
    OrientationHelper* mVerticalHelper;
    OrientationHelper* mHorizontalHelper;
private:
   int distanceToCenter(RecyclerView::LayoutManager& layoutManager, View& targetView, OrientationHelper& helper);
    int estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager,
        OrientationHelper& helper, int velocityX, int velocityY);
    View* findCenterView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
    float computeDistancePerChild(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
    OrientationHelper& getVerticalHelper(RecyclerView::LayoutManager& layoutManager);
    OrientationHelper& getHorizontalHelper(RecyclerView::LayoutManager& layoutManager);
public:
    PickerSnapHelper();
    ~PickerSnapHelper()override;
    void calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int distance[2])override;
    int findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY)override;
    View* findSnapView(RecyclerView::LayoutManager& layoutManager)override;
};
#endif/*__pickerSnapHelper_H__*/
