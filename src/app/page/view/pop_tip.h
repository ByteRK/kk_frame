/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-12-29 14:27:10
 * @FilePath: /kk_frame/src/app/page/view/pop_tip.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __POP_TIP_H__
#define __POP_TIP_H__

#include "base.h"

class TipPop :public PopBase {
private:
public:
    TipPop();
    ~TipPop();
    int8_t getType() const override;
protected:
    void initUI() override;
};

#endif // !__POP_TIP_H__