/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:53:34
 * @FilePath: /hana_frame/src/viewlibs/pickerSnapHelper.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

 
#include "pickerSnapHelper.h"

PickerSnapHelper::PickerSnapHelper() {
    mVerticalHelper = nullptr;
    mHorizontalHelper = nullptr;
}

PickerSnapHelper::~PickerSnapHelper() {
    delete mVerticalHelper;
    delete mHorizontalHelper;
}

void PickerSnapHelper::calculateDistanceToFinalSnap(
    RecyclerView::LayoutManager& layoutManager, View& targetView, int out[2]) {
    if (layoutManager.canScrollHorizontally()) {
        out[0] = distanceToCenter(layoutManager, targetView, getHorizontalHelper(layoutManager));
    } else {
        out[0] = 0;
    }

    if (layoutManager.canScrollVertically()) {
        out[1] = distanceToCenter(layoutManager, targetView, getVerticalHelper(layoutManager));
    } else {
        out[1] = 0;
    }
}

int PickerSnapHelper::findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY) {
    return RecyclerView::NO_POSITION;
}

View* PickerSnapHelper::findSnapView(RecyclerView::LayoutManager& layoutManager) {
    if (layoutManager.canScrollVertically()) {
        return findCenterView(layoutManager, getVerticalHelper(layoutManager));
    } else if (layoutManager.canScrollHorizontally()) {
        return findCenterView(layoutManager, getHorizontalHelper(layoutManager));
    }
    return nullptr;
}

int PickerSnapHelper::distanceToCenter(RecyclerView::LayoutManager& layoutManager,
    View& targetView, OrientationHelper& helper) {
    const int childCenter = helper.getDecoratedStart(&targetView)
        + (helper.getDecoratedMeasurement(&targetView) / 2);
    int containerCenter;
    if (layoutManager.getClipToPadding()) {
        containerCenter = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        containerCenter = helper.getEnd() / 2;
    }
    return childCenter - containerCenter;
}

int PickerSnapHelper::estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager,
    OrientationHelper& helper, int velocityX, int velocityY) {
    int distances[2];
    calculateScrollDistance(velocityX, velocityY, distances);
    float distancePerChild = computeDistancePerChild(layoutManager, helper);
    if (distancePerChild <= 0) {
        return 0;
    }
    const int distance =
        std::abs(distances[0]) > std::abs(distances[1]) ? distances[0] : distances[1];
    return (int)std::round(distance / distancePerChild);
}

View* PickerSnapHelper::findCenterView(RecyclerView::LayoutManager& layoutManager,
    OrientationHelper& helper) {
    int childCount = layoutManager.getChildCount();
    if (childCount == 0) {
        return nullptr;
    }

    View* closestChild = nullptr;
    int center;
    if (layoutManager.getClipToPadding()) {
        center = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        center = helper.getEnd() / 2;
    }
    int absClosest = INT_MAX;//Integer.MAX_VALUE;

    for (int i = 0; i < childCount; i++) {
        View* child = layoutManager.getChildAt(i);
        int childCenter = helper.getDecoratedStart(child)
            + (helper.getDecoratedMeasurement(child) / 2);
        int absDistance = std::abs(childCenter - center);

        /** if child center is closer than previous closest, set it as closest  **/
        if (absDistance < absClosest) {
            absClosest = absDistance;
            closestChild = child;
        }
    }
    return closestChild;
}

float PickerSnapHelper::computeDistancePerChild(RecyclerView::LayoutManager& layoutManager,
    OrientationHelper& helper) {
    View* minPosView = nullptr;
    View* maxPosView = nullptr;
    int minPos = INT_MAX;//Integer.MAX_VALUE;
    int maxPos = INT_MIN;//Integer.MIN_VALUE;
    int childCount = layoutManager.getChildCount();
    if (childCount == 0) {
        return INVALID_DISTANCE;
    }

    for (int i = 0; i < childCount; i++) {
        View* child = layoutManager.getChildAt(i);
        const int pos = layoutManager.getPosition(child);
        if (pos == RecyclerView::NO_POSITION) {
            continue;
        }
        if (pos < minPos) {
            minPos = pos;
            minPosView = child;
        }
        if (pos > maxPos) {
            maxPos = pos;
            maxPosView = child;
        }
    }
    if (minPosView == nullptr || maxPosView == nullptr) {
        return INVALID_DISTANCE;
    }
    int start = std::min(helper.getDecoratedStart(minPosView),
        helper.getDecoratedStart(maxPosView));
    int end = std::max(helper.getDecoratedEnd(minPosView),
        helper.getDecoratedEnd(maxPosView));
    int distance = end - start;
    if (distance == 0) {
        return INVALID_DISTANCE;
    }
    return 1.f * distance / ((maxPos - minPos) + 1);
}

OrientationHelper& PickerSnapHelper::getVerticalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mVerticalHelper == nullptr || mVerticalHelper->getLayoutManager() != &layoutManager) {
        mVerticalHelper = OrientationHelper::createVerticalHelper(&layoutManager);
    }
    return *mVerticalHelper;
}

OrientationHelper& PickerSnapHelper::getHorizontalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mHorizontalHelper == nullptr || mHorizontalHelper->getLayoutManager() != &layoutManager) {
        mHorizontalHelper = OrientationHelper::createHorizontalHelper(&layoutManager);
    }
    return *mHorizontalHelper;
}