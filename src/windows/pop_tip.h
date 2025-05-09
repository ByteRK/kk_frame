/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:13:30
 * @FilePath: /hana_frame/src/windows/pop_tip.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
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