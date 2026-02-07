/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:36:51
 * @LastEditTime: 2026-02-08 03:33:20
 * @FilePath: /kk_frame/src/app/page/components/wind_black.h
 * @Description: 息屏组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_BLACK_H__
#define __WIND_BLACK_H__

#include <view/view.h>

class WindBlack {
private:
    View*   mBlackView;         // 息屏组件
    bool    mIsInit;            // 是否初始化

public:
    WindBlack();
    ~WindBlack();
    
    void init(ViewGroup* parent);
    void showBlack();
    void hideBlack();
    bool isBlackShow();
    bool onKey(int keyCode, KeyEvent& evt, bool& result);

private:
    bool checkInit();
};


#endif // !__WIND_BLACK_H__
