/*
 * Copyright 2018 The Android Open Source Project
 * Copyright 2019 Rúben Sousa
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Port: Ricken
 * Time: 2024/04/08
 *
 */

#ifndef GRAVITYSNAPHELPER_H
#define GRAVITYSNAPHELPER_H

#include <view/view.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>
#include <widgetEx/recyclerview/linearsnaphelper.h>
#include <widgetEx/recyclerview/orientationhelper.h>
#include <widgetEx/recyclerview/recyclerview.h>

/**
 * A {@link LinearSnapHelper} that allows snapping to an edge or to the center.
 * <p>
 * Possible snap positions:
 * {@link Gravity#START}, {@link Gravity#TOP}, {@link Gravity#END}, {@link Gravity#BOTTOM},
 * {@link Gravity#CENTER}.
 * <p>
 * To customize the scroll duration, use {@link GravitySnapHelper#setScrollMsPerInch(float)}.
 * <p>
 * To customize the maximum scroll distance during flings,
 * use {@link GravitySnapHelper#setMaxFlingSizeFraction(float)}
 * or {@link GravitySnapHelper#setMaxFlingDistance(int)}
 */
class GravitySnapHelper :public LinearSnapHelper {
public:
    /**
     * A listener that's called when the {@link RecyclerView} used by {@link GravitySnapHelper}
     * changes its scroll state to {@link RecyclerView#SCROLL_STATE_IDLE}
     * and there's a valid snap position.
     */
    typedef std::function<void(int type)> SnapListener;

public:
    static constexpr int FLING_DISTANCE_DISABLE = -1;
    static constexpr float FLING_SIZE_FRACTION_DISABLE = -1.f;
private:
    // private int gravity;
    // private boolean isRtl;
    // private boolean snapLastItem;
    // private int nextSnapPosition;
    // private boolean isScrolling = false;
    // private boolean snapToPadding = false;
    // private float mScrollMsPerInch = 100f;
    // private int maxFlingDistance = FLING_DISTANCE_DISABLE;
    // private float mMaxFlingSizeFraction = FLING_SIZE_FRACTION_DISABLE;
    // private OrientationHelper verticalHelper;
    // private OrientationHelper horizontalHelper;
    // private GravitySnapHelper.SnapListener listener;
    // private RecyclerView recyclerView;

    int                mGravity;
    bool               mIsRtl;
    bool               mSnapLastItem;
    int                mNextSnapPosition;
    bool               mIsScrolling;
    bool               mSnapToPadding;
    float              mScrollMsPerInch;
    int                mMaxFlingDistance;
    float              mMaxFlingSizeFraction;
    OrientationHelper* mVerticalHelper;
    OrientationHelper* mHorizontalHelper;
    GravitySnapHelper::SnapListener mListener;

private:
    // RecyclerView::OnScrollListener* scrollListener = new RecyclerView::OnScrollListener() {
    //     @Override
    //         public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState) {
    //         super.onScrollStateChanged(recyclerView, newState);
    //         GravitySnapHelper.this.onScrollStateChanged(newState);
    //     }
    // };
    // void onScrollStateChanged(RecyclerView& recyclerView, int newState);

public:
    GravitySnapHelper(int gravity);

    GravitySnapHelper(int gravity, SnapListener snapListener);

    GravitySnapHelper(int gravity, bool enableSnapLastItem);

    GravitySnapHelper(int gravity, bool enableSnapLastItem, SnapListener snapListener);

    ~GravitySnapHelper();

    void init(int gravity, bool enableSnapLastItem, SnapListener snapListener);

    // @Override
    //     public void attachToRecyclerView(@Nullable RecyclerView recyclerView)
    //     throws IllegalStateException{
    // if (this.recyclerView != null) {
    //     this.recyclerView.removeOnScrollListener(scrollListener);
    // }
    // if (recyclerView != null) {
    //     recyclerView.setOnFlingListener(null);
    //     if (gravity == Gravity.START || gravity == Gravity.END) {
    //         isRtl = TextUtilsCompat.getLayoutDirectionFromLocale(Locale.getDefault())
    //                 == ViewCompat.LAYOUT_DIRECTION_RTL;
    //     }
    //     recyclerView.addOnScrollListener(scrollListener);
    //     this.recyclerView = recyclerView;
    // } else {
    //     this.recyclerView = null;
    // }
    // LinearSnapHelper::attachToRecyclerView(recyclerView);
    // }


    View* findSnapView(RecyclerView::LayoutManager& layoutManager)override;

    View* findSnapView(RecyclerView::LayoutManager& layoutManager, bool checkEdgeOfList);

    void calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int distance[2]);

    // @Override
    //     @NonNull
    //     public int[] calculateScrollDistance(int velocityX, int velocityY) {
    //     if (recyclerView == null
    //         || (verticalHelper == null && horizontalHelper == null)
    //         || (mMaxFlingDistance == FLING_DISTANCE_DISABLE
    //             && mMaxFlingSizeFraction == FLING_SIZE_FRACTION_DISABLE)) {
    //         return LinearSnapHelper::calculateScrollDistance(velocityX, velocityY);
    //     }
    //     final int[] out = new int[2];
    //     Scroller scroller = new Scroller(recyclerView.getContext(),
    //         new DecelerateInterpolator());
    //     int maxDistance = getFlingDistance();
    //     scroller.fling(0, 0, velocityX, velocityY,
    //         -maxDistance, maxDistance,
    //         -maxDistance, maxDistance);
    //     out[0] = scroller.getFinalX();
    //     out[1] = scroller.getFinalY();
    //     return out;
    // }

    // @Nullable
    //     @Override
    //     public RecyclerView.SmoothScroller createScroller(RecyclerView.LayoutManager layoutManager) {
    //     if (!(layoutManager instanceof RecyclerView.SmoothScroller.ScrollVectorProvider)
    //         || recyclerView == null) {
    //         return null;
    //     }
    //     return new LinearSmoothScroller(recyclerView.getContext()){
    //         @Override
    //         protected void onTargetFound(View targetView,
    //                                      RecyclerView.State state,
    //                                      RecyclerView.SmoothScroller.Action action) {
    //             if (recyclerView == null || recyclerView.getLayoutManager() == null) {
    //                 // The associated RecyclerView has been removed so there is no action to take.
    //                 return;
    //             }
    //             int[] snapDistances = calculateDistanceToFinalSnap(recyclerView.getLayoutManager(),
    //                     targetView);
    //             final int dx = snapDistances[0];
    //             final int dy = snapDistances[1];
    //             final int time = calculateTimeForDeceleration(Math.max(Math.abs(dx), Math.abs(dy)));
    //             if (time > 0) {
    //                 action.update(dx, dy, time, mDecelerateInterpolator);
    //             }
    //         }

    //         @Override
    //         protected float calculateSpeedPerPixel(DisplayMetrics displayMetrics) {
    //             return mScrollMsPerInch / displayMetrics.densityDpi;
    //         }
    //     };
    // }

    /**
     * Sets a {@link SnapListener} to listen for snap events
     *
     * @param listener a {@link SnapListener} that'll receive snap events or null to clear it
     */
    void setSnapListener(SnapListener listener);

    /**
     * Changes the mGravity of this {@link GravitySnapHelper}
     * and dispatches a smooth scroll for the new snap position.
     *
     * @param newGravity one of the following: {@link Gravity#START}, {@link Gravity#TOP},
     *                   {@link Gravity#END}, {@link Gravity#BOTTOM}, {@link Gravity#CENTER}
     * @param smooth     true if we should smooth scroll to new edge, false otherwise
     */
    void setGravity(int newGravity, bool smooth) ;

    /**
     * Updates the current view to be snapped
     *
     * @param smooth          true if we should smooth scroll, false otherwise
     * @param checkEdgeOfList true if we should check if we're at an edge of the list
     *                        and snap according to {@link GravitySnapHelper#getSnapLastItem()},
     *                        or false to force snapping to the nearest view
     */
    void updateSnap(bool smooth, bool checkEdgeOfList) ;

    /**
     * This method will only work if there's a ViewHolder for the given position.
     *
     * @return true if scroll was successful, false otherwise
     */
    bool scrollToPosition(int position) ;

    /**
     * Unlike {@link GravitySnapHelper#scrollToPosition(int)},
     * this method will generally always find a snap view if the position is valid.
     * <p>
     * The smooth scroller from {@link GravitySnapHelper#createScroller(RecyclerView.LayoutManager)}
     * will be used, and so will {@link GravitySnapHelper#mScrollMsPerInch} for the scroll velocity
     *
     * @return true if scroll was successful, false otherwise
     */
    bool smoothScrollToPosition(int position);
    /**
     * Get the current mGravity being applied
     *
     * @return one of the following: {@link Gravity#START}, {@link Gravity#TOP}, {@link Gravity#END},
     * {@link Gravity#BOTTOM}, {@link Gravity#CENTER}
     */
    int getGravity();

    /**
     * Changes the mGravity of this {@link GravitySnapHelper}
     * and dispatches a smooth scroll for the new snap position.
     *
     * @param newGravity one of the following: {@link Gravity#START}, {@link Gravity#TOP},
     *                   {@link Gravity#END}, {@link Gravity#BOTTOM}, {@link Gravity#CENTER}
     */
    void setGravity(int newGravity);

    /**
     * @return true if this SnapHelper should snap to the last item
     */
    bool getSnapLastItem();

    /**
     * Enable snapping of the last item that's snappable.
     * The default value is false, because you can't see the last item completely
     * if this is enabled.
     *
     * @param snap true if you want to enable snapping of the last snappable item
     */
    void setSnapLastItem(bool snap);

    /**
     * @return last distance set through {@link GravitySnapHelper#setMaxFlingDistance(int)}
     * or {@link GravitySnapHelper#FLING_DISTANCE_DISABLE} if we're not limiting the fling distance
     */
    int getMaxFlingDistance();

    /**
     * Changes the max fling distance in absolute values.
     *
     * @param distance max fling distance in pixels
     *                 or {@link GravitySnapHelper#FLING_DISTANCE_DISABLE}
     *                 to disable fling limits
     */
    void setMaxFlingDistance(int distance);

    /**
     * @return last distance set through {@link GravitySnapHelper#setMaxFlingSizeFraction(float)}
     * or {@link GravitySnapHelper#FLING_SIZE_FRACTION_DISABLE}
     * if we're not limiting the fling distance
     */
    float getMaxFlingSizeFraction() ;

    /**
     * Changes the max fling distance depending on the available size of the RecyclerView.
     * <p>
     * Example: if you pass 0.5f and the RecyclerView measures 600dp,
     * the max fling distance will be 300dp.
     *
     * @param fraction size fraction to be used for the max fling distance
     *                 or {@link GravitySnapHelper#FLING_SIZE_FRACTION_DISABLE}
     *                 to disable fling limits
     */
    void setMaxFlingSizeFraction(float fraction);

    /**
     * @return last scroll speed set through {@link GravitySnapHelper#setScrollMsPerInch(float)}
     * or 100f
     */
    float getScrollMsPerInch();

    /**
     * Sets the scroll duration in ms per inch.
     * <p>
     * Default value is 100.0f
     * <p>
     * This value will be used in
     * {@link GravitySnapHelper#createScroller(RecyclerView.LayoutManager)}
     *
     * @param ms scroll duration in ms per inch
     */
    void setScrollMsPerInch(float ms);

    /**
     * @return true if this SnapHelper should snap to the padding. Defaults to false.
     */
    bool getSnapToPadding();

    /**
     * If true, GravitySnapHelper will snap to the mGravity edge
     * plus any amount of padding that was set in the RecyclerView.
     * <p>
     * The default value is false.
     *
     * @param snapToPadding true if you want to snap to the padding
     */
    void setSnapToPadding(bool sp);

    /**
     * @return the position of the current view that's snapped
     * or {@link RecyclerView#NO_POSITION} in case there's none.
     */
    int getCurrentSnappedPosition() ;

private:
    int getFlingDistance();

    /**
     * @return true if the scroll will snap to a view, false otherwise
     */
    bool scrollTo(int position, bool smooth);

    int getDistanceToStart(View* targetView, OrientationHelper* helper) ;

    int getDistanceToEnd(View* targetView, OrientationHelper* helper);

    /**
     * Returns the first view that we should snap to.
     *
     * @param layoutManager the RecyclerView's LayoutManager
     * @param helper        orientation helper to calculate view sizes
     * @param mGravity       gravity to find the closest view
     * @return the first view in the LayoutManager to snap to, or null if we shouldn't snap to any
     */
    View* findView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper,
        int gravity, bool checkEdgeOfList);

    bool isAtEdgeOfList(LinearLayoutManager lm) ;

    /**
     * Dispatches a {@link SnapListener#onSnap(int)} event if the snapped position
     * is different than {@link RecyclerView#NO_POSITION}.
     * <p>
     * When {@link GravitySnapHelper#findSnapView(RecyclerView.LayoutManager)} returns null,
     * {@link GravitySnapHelper#dispatchSnapChangeWhenPositionIsUnknown()} is called
     *
     * @param newState the new RecyclerView scroll state
     */
    void onScrollStateChanged(int newState);

    /**
     * Calls {@link GravitySnapHelper#findSnapView(RecyclerView.LayoutManager, boolean)}
     * without the check for the edge of the list.
     * <p>
     * This makes sure that a position is reported in {@link SnapListener#onSnap(int)}
     */
    void dispatchSnapChangeWhenPositionIsUnknown() ;

    OrientationHelper& getVerticalHelper(RecyclerView::LayoutManager& layoutManager) ;

    OrientationHelper& getHorizontalHelper(RecyclerView::LayoutManager& layoutManager) ;

};

#endif //GRAVITYSNAPHELPER_H