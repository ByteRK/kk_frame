/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-05-31 16:56:29
 * @FilePath: /cy_frame/src/viewlibs/rkLayoutManage.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */


#ifndef __RK_LAYOUTMANAGER_H__
#define __RK_LAYOUTMANAGER_H__

#include "pickerLayoutManager.h"

 /// @brief 元素管理
class RKLayoutManage :public PickerLayoutManager {
public:
    enum { // SnapHelper类型
        SNAPHELPER_FAST,    // 快速（默认）
        SNAPHELPER_PAGER,   // 分页
        SNAPHELPER_LINEAR,  // 线性
    };

    enum { // XY轴偏移类型  相对于Position
        TRANSFER_NEGATE = -1,       // 负值
        TRANSFER_ABS = 0,           // 绝对值
        TRANSFER_RELATIVE = 1,      // 相对值
    };

    typedef struct Transform_Struct {
        float       position;    // 位置
        float       scale;       // 缩放
        float       alpha;       // 透明度
        int         transX;      // X轴偏移量
        int         transXType;  // X轴偏移类型
        int         transY;      // Y轴偏移量
        int         transYType;  // Y轴偏移类型
        Transform_Struct() :
            position(0.f), scale(1.f), alpha(1.f),
            transX(0), transXType(TRANSFER_NEGATE), transY(0), transYType(TRANSFER_ABS) {
        };
        Transform_Struct(float p, float s, float a,
            int tx, int txt, int ty, int tyt) :
            position(p), scale(s), alpha(a),
            transX(tx), transXType(txt), transY(ty), transYType(tyt) {
        }
    }TransformStruct;

    typedef std::function<void(int, int)> OnCenterViewChangeListener;
protected:
    std::vector<TransformStruct> mTransformList;
    
    int   mOldCenterPosition;
    View* mOldCenterView;
    OnCenterViewChangeListener mOnCenterViewChangeListener;
private:
    long  mExpandAnimDuration;    // 展开动画时长
    bool  mEnableExpandAnim;      // 是否启用展开动画
    bool  mExpandAnimFinish;      // 展开动画是否完成
    Animator::AnimatorListener mAnimatorListener;
public:
    RKLayoutManage(Context* context, int orientation, bool reverseLayout);
    RKLayoutManage(Context* context, RecyclerView* recyclerView, int orientation, bool reverseLayout, int itemCount, bool isAlpha);
    ~RKLayoutManage();

    /// @brief 运行展开动画
    void runExpandAnim();

    /// @brief 设置是否启用展开动画
    /// @param enable 
    void setEnableExpandAnim(bool enable);

    /// @brief 设置展开动画时长
    /// @param duration 
    void setExpandAnimDuration(long duration);

    /// @brief 设置SnapHelper
    /// @param snapHelperType SNAPHELPER_FAST SNAPHELPER_PAGER SNAPHELPER_LINEAR
    void setSnapHelper(int snapHelperType);

    /// @brief 设置变换参数列表
    /// @param transformList 
    void setTransformList(const std::vector<TransformStruct>& transformList);

    /// @brief 设置中心view切换监听
    /// @brief 滑动过程中响应
    /// @param listener 
    void setOnCenterViewChangeListener(OnCenterViewChangeListener listener);
protected:
    /// @brief 初始化
    virtual void init();

    /// @brief 横向情况下的缩放
    virtual void scaleHorizontalChildView() override;

    /// @brief 竖向方向上的缩放
    virtual void scaleVerticalChildView() override;

    /// @brief 计算缩放值
    virtual void calculateScaleValue(const float& position, float& scaleFactor, float& alphaFactor, float& indentX, float& indentY);

    /// @brief 开始缩放
    virtual void startChildScale(View* child, const float& abs, const float& offset,
        const  float& scaleFactor, const  float& alphaFactor, const  float& indentX, const  float& indentY);

private:
    /// @brief 调整布局
    /// @param recycler 
    /// @param state 
    void onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state) override;
};

#endif