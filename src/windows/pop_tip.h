/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-02-18 19:54:51
 * @FilePath: /cy_frame/src/windows/pop_tip.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
 *
 */

#ifndef _POP_TIP_H_
#define _POP_TIP_H_

#include "base.h"

class TipPop :public PopBase {
private:
public:
    TipPop();
    ~TipPop();
    uint8_t getType() const override;
protected:
    void initUI() override;
};

#endif // !_POP_TIP_H_