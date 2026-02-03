/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2025-08-20 18:25:33
 * @FilePath: /kk_frame/src/viewlibs/rkLayoutManage.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
**/


#ifndef __RK_LAYOUTMANAGER_H__
#define __RK_LAYOUTMANAGER_H__

#include <widgetEx/recyclerview/recyclerview.h>
#include <widgetEx/recyclerview/snaphelper.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>

 /// @brief 元素管理
class RKLayoutManage :public LinearLayoutManager {
public:
    typedef enum { /// 计算模式
        MEASURE_BY_COUND,    // 根据显示数量计算recycleview Padding
        MEASURE_BY_CENTER,   // 根据中间项宽高以及recycleview宽高进行计算 （viewflipper不适用）
    } RKMeasureMode;
    typedef enum { // SnapHelper类型
        SNAPHELPER_FAST,    // 快速（默认）
        SNAPHELPER_PAGER,   // 分页
        SNAPHELPER_LINEAR,  // 线性
    } RKSnapHelperType;

    typedef enum { // XY轴偏移类型  相对于Position
        TRANSFER_NEGATE = -1,       // 负值
        TRANSFER_ABS = 0,           // 绝对值
        TRANSFER_RELATIVE = 1,      // 相对值
    } RKTransferType;

    typedef struct Transform_Struct { // 变换参数
        float           position;    // 位置
        float           scale;       // 缩放
        float           alpha;       // 透明度
        int             transX;      // X轴偏移量
        RKTransferType  transXType;  // X轴偏移类型
        int             transY;      // Y轴偏移量
        RKTransferType  transYType;  // Y轴偏移类型
        Transform_Struct() :
            position(0.f), scale(1.f), alpha(1.f),
            transX(0), transXType(TRANSFER_NEGATE),
            transY(0), transYType(TRANSFER_ABS) {
        };
        Transform_Struct(float p, float s, float a,
            int tx, RKTransferType txt, int ty, RKTransferType tyt) :
            position(p), scale(s), alpha(a),
            transX(tx), transXType(txt), transY(ty), transYType(tyt) {
        }
    } TransformStruct;

    typedef std::function<void(int, int)> OnCenterChangingListener;
    typedef std::function<void(View*, int)> OnCenterChangedListener;
protected:
    RecyclerView*                mRecyclerView;               // RV
    SnapHelper*                  mSnapHelper;                 // SnapHelper
    int                          mCenterPosition;             // 中间项序号

    bool                         mIsAlpha;                    // 是否启用透明度
    int                          mDisplayCount;               // 显示数量
    int                          mOrientation;                // 方向
    int                          mMeasureMode;                // 计算模式
    std::vector<TransformStruct> mTransformList;              // 变换参数列表
    bool                         mScrollVerticallyEnable;     // 是否启用垂直滚动
    bool                         mScrollHorizontallyEnable;   // 是否启用水平滚动

    OnCenterChangingListener     mOnCenterChangingListener;   // 滑动回调
    OnCenterChangedListener      mOnCenterChangedListener;    // 滑动回调
private:
    long  mExpandAnimDuration;                         // 展开动画时长
    bool  mExpandAnimEnable;                           // 是否启用展开动画
    bool  mExpandAnimFinish;                           // 展开动画是否完成
    Animator::AnimatorListener   ExpandAnimListener;   // 展开动画监听
public:
    RKLayoutManage(Context* context, RecyclerView* recyclerView, int orientation, bool reverseLayout, int itemCount, bool isAlpha = true);
    ~RKLayoutManage();

    /// @brief 运行展开动画
    void runExpandAnim();

    /// @brief 设置是否启用展开动画
    /// @param enable 
    void setExpandAnimEnable(bool enable);

    /// @brief 设置展开动画时长
    /// @param duration 
    void setExpandAnimDuration(long duration);

    /// @brief 设置计算模式
    /// @param measureMode MEASURE_BY_COUND MEASURE_BY_CENTER
    void setMeasureMode(RKMeasureMode measureMode);

    /// @brief 设置SnapHelper
    /// @param snapHelperType SNAPHELPER_FAST SNAPHELPER_PAGER SNAPHELPER_LINEAR
    void setSnapHelper(RKSnapHelperType snapHelperType);

    /// @brief 设置变换参数列表
    /// @param transformList 
    void setTransformList(const std::vector<TransformStruct>& transformList);

    /// @brief 设置中心view切换监听
    /// @brief 滑动过程中响应
    /// @param listener
    void setOnCenterChangingListener(OnCenterChangingListener listener);

    /// @brief 设置中心view切换监听
    /// @brief 滑动过程结束响应
    /// @param listener 
    void setOnCenterChangedListener(OnCenterChangedListener listener);

    /// @brief 设置是否启用水平滚动
    /// @param scrollHorizontallyEnable 
    void setScrollHorizontallyEnable(bool scrollHorizontallyEnable);

    /// @brief 设置是否启用垂直滚动
    /// @param scrollVerticallyEnable
    void setScrollVerticallyEnable(bool scrollVerticallyEnable);

    /// @brief 获取中间项序号
    int getCenterPosition();

public:
    /// @brief 初始化
    void init();

    /// @brief 滑动状态改变
    /// @param state 
    void onScrollStateChanged(int state)override;

    /// @brief 测量时
    /// @param recycler 
    /// @param state 
    /// @param widthSpec 
    /// @param heightSpec 
    void onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec)override;

    /// @brief 布局完成
    /// @param state 
    void onLayoutCompleted(RecyclerView::State& state)override;

    /// @brief 横向滑动
    /// @param dx 
    /// @param recycler 
    /// @param state 
    /// @return 
    int  scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;

    /// @brief 纵向滑动
    /// @param dy 
    /// @param recycler 
    /// @param state 
    /// @return 
    int  scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;

    /// @brief 调整横向时的子view
    void adjustHorizontalChildView();

    /// @brief 调整纵向时的子view
    void adjustVerticalChildView();

    /// @brief 计算缩放值
    /// @param position
    /// @param scaleFactor
    /// @param alphaFactor
    /// @param indentX
    /// @param indentY
    void calculateScaleValue(const float& position, float& scaleFactor, float& alphaFactor, float& indentX, float& indentY);

    /// @brief 开始缩放
    /// @brief 需要调用calculateScaleValue计算缩放值
    /// @param child
    /// @param abs
    /// @param offset
    /// @param scaleFactor
    /// @param alphaFactor
    /// @param indentX
    /// @param indentY
    void startChildScale(View* child, const float& abs, const float& offset,
        const  float& scaleFactor, const  float& alphaFactor, const  float& indentX, const  float& indentY);

};

#endif