/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-02-08 04:02:25
 * @FilePath: /kk_frame/src/app/page/components/wind_pop.h
 * @Description: 弹窗组件
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIND_POP_H__
#define __WIND_POP_H__

#include "pop.h"

class WindPop {
private:
    PopBase*   mPop;            // 弹窗指针
    ViewGroup* mPopBox;         // 弹窗容器
    int64_t    mPopNextTick;    // 弹窗下次Tick时间

public:
    WindPop();
    ~WindPop();

    void       init(ViewGroup* parent);
    void       onTick();
    int64_t    getPopNextTick();

    PopBase*   getPop();
    int8_t     getPopType();
    int8_t     showPop(PopBase* pop, LoadMsgBase* initData = nullptr);
    void       removePop();
    void       hidePopBox();
    void       showPopBox();
    bool       onKey(int keyCode, KeyEvent& evt, bool& result);
    
};

#endif // !__WIND_POP_H__