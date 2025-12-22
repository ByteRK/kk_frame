#include "gravitySnapHelper.h"
#include <limits> // to replay Integer.MAX_VALUE

GravitySnapHelper::GravitySnapHelper(int gravity) {
    GravitySnapHelper(gravity, false, nullptr);
}

GravitySnapHelper::GravitySnapHelper(int gravity, SnapListener snapListener) {
    init(gravity, false, snapListener);
}

GravitySnapHelper::GravitySnapHelper(int gravity, bool enableSnapLastItem) {
    init(gravity, enableSnapLastItem, nullptr);
}

GravitySnapHelper::GravitySnapHelper(int gravity, bool enableSnapLastItem, SnapListener snapListener) {
    init(gravity, enableSnapLastItem, snapListener);
}

GravitySnapHelper::~GravitySnapHelper() {
    if (mVerticalHelper)delete mVerticalHelper;
    if (mHorizontalHelper)delete mHorizontalHelper;
}

void GravitySnapHelper::init(int gravity, bool enableSnapLastItem, SnapListener snapListener) {
    if (gravity != Gravity::START
        && gravity != Gravity::END
        && gravity != Gravity::BOTTOM
        && gravity != Gravity::TOP
        && gravity != Gravity::CENTER) {
        LOGE("Invalid gravity value. Use START | END | BOTTOM | TOP | CENTER constants");
    }
    mIsRtl = false;
    mGravity = gravity;
    mSnapLastItem = enableSnapLastItem;
    mNextSnapPosition = RecyclerView::NO_POSITION;
    mIsScrolling = false;
    mSnapToPadding = false;
    mScrollMsPerInch = 100.f;
    mMaxFlingDistance = FLING_DISTANCE_DISABLE;
    mMaxFlingSizeFraction = FLING_SIZE_FRACTION_DISABLE;
    mVerticalHelper = nullptr;
    mHorizontalHelper = nullptr;
    mListener = snapListener;
}

View* GravitySnapHelper::findSnapView(RecyclerView::LayoutManager& layoutManager) {
    return findSnapView(layoutManager, true);
}

View* GravitySnapHelper::findSnapView(RecyclerView::LayoutManager& layoutManager, bool checkEdgeOfList) {
    View* snapView = nullptr;
    LOGE("mGravity = %d", mGravity);

    switch (mGravity) {
    case Gravity::START:
        snapView = findView(layoutManager, getHorizontalHelper(layoutManager), Gravity::START, checkEdgeOfList);
        break;
    case Gravity::END:
        snapView = findView(layoutManager, getHorizontalHelper(layoutManager), Gravity::END, checkEdgeOfList);
        break;
    case Gravity::TOP:
        snapView = findView(layoutManager, getVerticalHelper(layoutManager), Gravity::START, checkEdgeOfList);
        break;
    case Gravity::BOTTOM:
        snapView = findView(layoutManager, getVerticalHelper(layoutManager), Gravity::END, checkEdgeOfList);
        break;
    case Gravity::CENTER:
        if (layoutManager.canScrollHorizontally()) {
            snapView = findView(layoutManager, getHorizontalHelper(layoutManager), Gravity::CENTER, checkEdgeOfList);
        } else {
            snapView = findView(layoutManager, getVerticalHelper(layoutManager), Gravity::CENTER, checkEdgeOfList);
        }
        break;
    }
    if (snapView != nullptr) {
        mNextSnapPosition = mRecyclerView->getChildAdapterPosition(snapView);
    } else {
        mNextSnapPosition = RecyclerView::NO_POSITION;
    }
    return snapView;
}

void GravitySnapHelper::calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int distance[2]) {
    if (mGravity == Gravity::CENTER) {
        //noinspection ConstantConditions
        return LinearSnapHelper::calculateDistanceToFinalSnap(layoutManager, targetView, distance);
    }

    // LinearLayoutManager* lm = &(LinearLayoutManager&)layoutManager;

    if (layoutManager.canScrollHorizontally()) {
        if ((mIsRtl && mGravity == Gravity::END) || (!mIsRtl && mGravity == Gravity::START)) {
            distance[0] = getDistanceToStart(&targetView, &getHorizontalHelper(layoutManager));
        } else {
            distance[0] = getDistanceToEnd(&targetView, &getHorizontalHelper(layoutManager));
        }
    } else if (layoutManager.canScrollVertically()) {
        if (mGravity == Gravity::TOP) {
            distance[1] = getDistanceToStart(&targetView, &getVerticalHelper(layoutManager));
        } else {
            distance[1] = getDistanceToEnd(&targetView, &getVerticalHelper(layoutManager));
        }
    }
}

void GravitySnapHelper::setSnapListener(SnapListener listener) {
    mListener = listener;
}

void GravitySnapHelper::setGravity(int newGravity, bool smooth) {
    if (mGravity != newGravity) {
        mGravity = newGravity;
        updateSnap(smooth, false);
    }
}

void GravitySnapHelper::updateSnap(bool smooth, bool checkEdgeOfList) {
    if (mRecyclerView == nullptr || mRecyclerView->getLayoutManager() == nullptr) {
        return;
    }
    RecyclerView::LayoutManager* lm = mRecyclerView->getLayoutManager();
    View* snapView = findSnapView(*lm, checkEdgeOfList);
    if (snapView != nullptr) {
        int out[2] = { 0 };
        calculateDistanceToFinalSnap(*lm, *snapView, out);
        if (smooth) {
            mRecyclerView->smoothScrollBy(out[0], out[1]);
        } else {
            mRecyclerView->scrollBy(out[0], out[1]);
        }
    }
}

bool GravitySnapHelper::scrollToPosition(int position) {
    if (position == RecyclerView::NO_POSITION) {
        return false;
    }
    return scrollTo(position, false);
}

bool GravitySnapHelper::smoothScrollToPosition(int position) {
    if (position == RecyclerView::NO_POSITION) {
        return false;
    }
    return scrollTo(position, true);
}

int GravitySnapHelper::getGravity() {
    return mGravity;
}

void GravitySnapHelper::setGravity(int newGravity) {
    setGravity(newGravity, true);
}

bool GravitySnapHelper::getSnapLastItem() {
    return mSnapLastItem;
}

void GravitySnapHelper::setSnapLastItem(bool snap) {
    mSnapLastItem = snap;
}

int GravitySnapHelper::getMaxFlingDistance() {
    return mMaxFlingDistance;
}

void GravitySnapHelper::setMaxFlingDistance(int distance) {
    mMaxFlingDistance = distance;
    mMaxFlingSizeFraction = FLING_SIZE_FRACTION_DISABLE;
}

float GravitySnapHelper::getMaxFlingSizeFraction() {
    return mMaxFlingSizeFraction;
}

void GravitySnapHelper::setMaxFlingSizeFraction(float fraction) {
    mMaxFlingDistance = FLING_DISTANCE_DISABLE;
    mMaxFlingSizeFraction = fraction;
}

float GravitySnapHelper::getScrollMsPerInch() {
    return mScrollMsPerInch;
}

void GravitySnapHelper::setScrollMsPerInch(float ms) {
    mScrollMsPerInch = ms;
}

bool GravitySnapHelper::getSnapToPadding() {
    return mSnapToPadding;
}

void GravitySnapHelper::setSnapToPadding(bool sp) {
    mSnapToPadding = sp;
}

int GravitySnapHelper::getCurrentSnappedPosition() {
    if (mRecyclerView != nullptr && mRecyclerView->getLayoutManager() != nullptr) {
        View* snappedView = findSnapView(*(mRecyclerView->getLayoutManager()));
        if (snappedView != nullptr) {
            return mRecyclerView->getChildAdapterPosition(snappedView);
        }
    }
    return RecyclerView::NO_POSITION;
}

int GravitySnapHelper::getFlingDistance() {
    if (mMaxFlingSizeFraction != FLING_SIZE_FRACTION_DISABLE) {
        if (mVerticalHelper != nullptr) {
            return (int)(mRecyclerView->getHeight() * mMaxFlingSizeFraction);
        } else if (mHorizontalHelper != nullptr) {
            return (int)(mRecyclerView->getWidth() * mMaxFlingSizeFraction);
        } else {
            // return Integer.MAX_VALUE;
            return std::numeric_limits<int>::max();
        }
    } else if (mMaxFlingDistance != FLING_DISTANCE_DISABLE) {
        return mMaxFlingDistance;
    } else {
        // return Integer.MAX_VALUE;
        return std::numeric_limits<int>::max();
    }
}

bool GravitySnapHelper::scrollTo(int position, bool smooth) {
    if (mRecyclerView->getLayoutManager() != nullptr) {
        if (smooth) {
            RecyclerView::SmoothScroller* smoothScroller
                = createScroller(*mRecyclerView->getLayoutManager());
            if (smoothScroller != nullptr) {
                smoothScroller->setTargetPosition(position);
                mRecyclerView->getLayoutManager()->startSmoothScroll(smoothScroller);
                return true;
            }
        } else {
            RecyclerView::ViewHolder* viewHolder
                = mRecyclerView->findViewHolderForAdapterPosition(position);
            if (viewHolder != nullptr) {
                int distances[2] = { 0 };
                calculateDistanceToFinalSnap(*mRecyclerView->getLayoutManager(),
                    *(viewHolder->itemView), distances);
                mRecyclerView->scrollBy(distances[0], distances[1]);
                return true;
            }
        }
    }
    return false;
}

int GravitySnapHelper::getDistanceToStart(View* targetView, OrientationHelper* helper) {
    int distance;
    // If we don't care about padding, just snap to the start of the view
    if (!mSnapToPadding) {
        int childStart = helper->getDecoratedStart(targetView);
        if (childStart >= helper->getStartAfterPadding() / 2) {
            distance = childStart - helper->getStartAfterPadding();
        } else {
            distance = childStart;
        }
    } else {
        distance = helper->getDecoratedStart(targetView) - helper->getStartAfterPadding();
    }
    return distance;
}

int GravitySnapHelper::getDistanceToEnd(View* targetView, OrientationHelper* helper) {
    int distance;

    if (!mSnapToPadding) {
        int childEnd = helper->getDecoratedEnd(targetView);
        if (childEnd >= helper->getEnd() - (helper->getEnd() - helper->getEndAfterPadding()) / 2) {
            distance = helper->getDecoratedEnd(targetView) - helper->getEnd();
        } else {
            distance = childEnd - helper->getEndAfterPadding();
        }
    } else {
        distance = helper->getDecoratedEnd(targetView) - helper->getEndAfterPadding();
    }

    return distance;
}

View* GravitySnapHelper::findView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper,
    int gravity, bool checkEdgeOfList) {

    if (layoutManager.getChildCount() == 0) {
        return nullptr;
    }

    // If we're at an edge of the list, we shouldn't snap
    // to avoid having the last item not completely visible.
    if (checkEdgeOfList && (isAtEdgeOfList((LinearLayoutManager&)layoutManager) && !mSnapLastItem)) {
        return nullptr;
    }

    View* edgeView = nullptr;
    // int distanceToTarget = Integer.MAX_VALUE;
    int distanceToTarget = std::numeric_limits<int>::max();
    int center;
    if (layoutManager.getClipToPadding()) {
        center = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        center = helper.getEnd() / 2;
    }

    const bool snapToStart = (gravity == Gravity::START && !mIsRtl)
        || (gravity == Gravity::END && mIsRtl);

    const bool snapToEnd = (gravity == Gravity::START && mIsRtl)
        || (gravity == Gravity::END && !mIsRtl);

    LOGE("layoutManager.getChildCount() = %d", layoutManager.getChildCount());
    for (int i = 0; i < layoutManager.getChildCount(); i++) {
        LOGD("in i = %d",i);
        View* currentView = layoutManager.getChildAt(i);
        int currentViewDistance;
        if (snapToStart) {
            if (!mSnapToPadding) {
                currentViewDistance = std::abs(helper.getDecoratedStart(currentView));
            } else {
                currentViewDistance = std::abs(helper.getStartAfterPadding()
                    - helper.getDecoratedStart(currentView));
            }
        } else if (snapToEnd) {
            if (!mSnapToPadding) {
                currentViewDistance = std::abs(helper.getDecoratedEnd(currentView)
                    - helper.getEnd());
            } else {
                currentViewDistance = std::abs(helper.getEndAfterPadding()
                    - helper.getDecoratedEnd(currentView));
            }
        } else {
            currentViewDistance = std::abs(helper.getDecoratedStart(currentView)
                + (helper.getDecoratedMeasurement(currentView) / 2) - center);
        }
        if (currentViewDistance < distanceToTarget) {
            distanceToTarget = currentViewDistance;
            edgeView = currentView;
        }
    }
    return edgeView;
}

bool GravitySnapHelper::isAtEdgeOfList(LinearLayoutManager lm) {
    if ((!lm.getReverseLayout() && mGravity == Gravity::START)
        || (lm.getReverseLayout() && mGravity == Gravity::END)
        || (!lm.getReverseLayout() && mGravity == Gravity::TOP)
        || (lm.getReverseLayout() && mGravity == Gravity::BOTTOM)) {
        return lm.findLastCompletelyVisibleItemPosition() == lm.getItemCount() - 1;
    } else if (mGravity == Gravity::CENTER) {
        return lm.findFirstCompletelyVisibleItemPosition() == 0
            || lm.findLastCompletelyVisibleItemPosition() == lm.getItemCount() - 1;
    } else {
        return lm.findFirstCompletelyVisibleItemPosition() == 0;
    }
}

void GravitySnapHelper::onScrollStateChanged(int newState) {
    if (newState == RecyclerView::SCROLL_STATE_IDLE && mListener != nullptr) {
        if (mIsScrolling) {
            if (mNextSnapPosition != RecyclerView::NO_POSITION) {
                // mListener.onSnap(mNextSnapPosition);
                mListener(mNextSnapPosition);
            } else {
                dispatchSnapChangeWhenPositionIsUnknown();
            }
        }
    }
    mIsScrolling = newState != RecyclerView::SCROLL_STATE_IDLE;
}

void GravitySnapHelper::dispatchSnapChangeWhenPositionIsUnknown() {
    RecyclerView::LayoutManager* layoutManager = mRecyclerView->getLayoutManager();
    if (layoutManager == nullptr) {
        return;
    }
    View* snapView = findSnapView(*layoutManager, false);
    if (snapView == nullptr) {
        return;
    }
    int snapPosition = mRecyclerView->getChildAdapterPosition(snapView);
    if (snapPosition != RecyclerView::NO_POSITION && mListener) {
        // listener.onSnap(snapPosition);
        mListener(snapPosition);
    }
}

OrientationHelper& GravitySnapHelper::getVerticalHelper(RecyclerView::LayoutManager& layoutManager) {

        if (mVerticalHelper == nullptr || mVerticalHelper->getLayoutManager() != &layoutManager) {
        mVerticalHelper = OrientationHelper::createVerticalHelper(&layoutManager);
    }
    return *mVerticalHelper;
}

OrientationHelper& GravitySnapHelper::getHorizontalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mHorizontalHelper == nullptr || mHorizontalHelper->getLayoutManager() != &layoutManager) {
        mHorizontalHelper = OrientationHelper::createHorizontalHelper(&layoutManager);
    }
    return *mHorizontalHelper;
}
