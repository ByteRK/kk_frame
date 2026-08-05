/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-08-05 16:44:20
 * @LastEditTime: 2026-08-05 16:53:34
 * @FilePath: /kk_frame/src/class/recycleview_space.h
 * @Description: Recycleview 间距类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __RECYCLEVIEW_SPACE_H__
#define __RECYCLEVIEW_SPACE_H__

#include <widgetEx/recyclerview/recyclerview.h>

/// @brief Recycleview 间距类
class RecyclerViewItemSpace : public RecyclerView::ItemDecoration {
public:
    typedef enum {
        LEFT = 0,
        TOP,
    } Direction;

private:
    int       mSpace{ 20 };        // 默认为20
    Direction mDirection{ LEFT };  // 默认为左边

public:
    RecyclerViewItemSpace() { }
    RecyclerViewItemSpace(int space, Direction direction = LEFT) :mSpace(space), mDirection(direction) { }
    void getItemOffsets(Rect& outRect, View& view, RecyclerView& parent, RecyclerView::State& state) override {
        if (parent.getChildPosition(&view) > 0) { // 排除第一个
            switch (mDirection) {
            case LEFT: outRect.left += mSpace; break;
            case TOP:  outRect.top += mSpace;  break;
            default:                           break;
            }
        }
    }
};

#endif // __RECYCLEVIEW_SPACE_H__
